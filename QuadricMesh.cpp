#include "QuadricMesh.h"
#include "d3dx12.h"
#include "Helpers.h"
#include "DX12.h"
#include <iostream>

using namespace DirectX;

void QuadricMesh::UpdateMeshData() const
{
    MeshData m{};

    m.transform = XMMatrixAffineTransformation(XMLoadFloat3(&m_Transform.scale), XMVectorZero()
        , XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_Transform.rotation))
        , XMLoadFloat3(&m_Transform.position));
    m.transform = XMMatrixInverse(nullptr, m.transform);
    m.numQuadrics = (unsigned int)m_Quadrics.size();

    BYTE* mapped = nullptr;
    m_MeshDataBuffer->Map(0, nullptr,
        reinterpret_cast<void**>(&mapped));
    memcpy(mapped, &m, sizeof(MeshData));
    if (m_MeshDataBuffer != nullptr)
        m_MeshDataBuffer->Unmap(0, nullptr);
}

QuadricMesh::QuadricMesh(DX12* pDX12, const std::vector<InQuadric>& quadrics, const Transform& transform)
    :m_Quadrics{quadrics},  m_Transform{transform}
{
    size_t byteSize = sizeof(InQuadric) * m_Quadrics.size();
    CD3DX12_HEAP_PROPERTIES properties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
    CD3DX12_RESOURCE_DESC desc{ CD3DX12_RESOURCE_DESC::Buffer(byteSize) };

    // Create the actual default buffer resource.
    ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_InputBuffer.GetAddressOf())));

    properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_InputUploadBuffer.GetAddressOf())));
    
    UpdateBuffers(pDX12);


    // Create the buffer that will be a UAV with outputquadrics
    byteSize = sizeof(MeshOutputData);
    properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_MeshOutputBuffer)));

    //uploadBuffer
    properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
    desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
    ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_MeshOutputResetBuffer)));

    MeshOutputData* mapped = nullptr;
    m_MeshOutputResetBuffer->Map(0, nullptr,
        reinterpret_cast<void**>(&mapped));
    mapped->numOutputQuadrics = (UINT)m_Quadrics.size();
    mapped->overflowed = false;
    if (m_MeshOutputResetBuffer != nullptr)
        m_MeshOutputResetBuffer->Unmap(0, nullptr);

    //readback
    properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK);
    desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_NONE);
    ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_MeshOutputReadBackBuffer)));
    m_MeshOutputReadBackBuffer->SetName(LPCWSTR(L"m_MeshOutputReadBackBuffer"));
    m_MeshOutputResetBuffer->SetName(LPCWSTR(L"m_MeshOutputResetBuffer"));
    m_MeshOutputBuffer->SetName(LPCWSTR(L"m_MeshOutputBuffer"));


    //create transform data
    //Constant Buffer
    properties = { CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
    desc = { CD3DX12_RESOURCE_DESC::Buffer((sizeof(MeshData) + 255) & ~255) };
    pDX12->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_MeshDataBuffer));

    //set data
    UpdateMeshData();

    auto pPipeline = pDX12->GetPipeline();
    ThrowIfFailed(pPipeline->commandAllocator->Reset());
    ThrowIfFailed(pPipeline->commandList->Reset(pPipeline->commandAllocator.Get(), nullptr));

    pPipeline->commandList->CopyResource(m_MeshOutputBuffer.Get(), m_MeshOutputResetBuffer.Get());

    CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshOutputBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);

    pPipeline->commandList->ResourceBarrier(1 , &transition);

    pPipeline->commandList->CopyResource(m_MeshOutputReadBackBuffer.Get() , m_MeshOutputBuffer.Get());

    transition = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshOutputBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    pPipeline->commandList->ResourceBarrier(1, &transition);

    ThrowIfFailed(pPipeline->commandList->Close());
    ID3D12CommandList* cmdsLists[] = { pPipeline->commandList.Get() };
    pPipeline->commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    pPipeline->Flush();

}

void QuadricMesh::ReadMeshOutput()
{
    // Map the data so we can read it on CPU.
    MeshOutputData* mappedData = nullptr;
    ThrowIfFailed(m_MeshOutputReadBackBuffer->Map(0, nullptr,
        reinterpret_cast<void**>(&mappedData)));

    m_OutputData = *mappedData;

    m_MeshOutputReadBackBuffer->Unmap(0, nullptr);
}


void QuadricMesh::UpdateBuffers(DX12* pDX12)
{
    size_t byteSize = sizeof(InQuadric) * m_Quadrics.size();

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = m_Quadrics.data();
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    auto pPipeline = pDX12->GetPipeline();
    ThrowIfFailed(pPipeline->commandAllocator->Reset());
    ThrowIfFailed(pPipeline->commandList->Reset(pPipeline->commandAllocator.Get(), nullptr));
    auto pComList = pPipeline->commandList;
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_InputBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    pComList->ResourceBarrier(1, &barrier);

    UpdateSubresources<1>(pComList.Get(), m_InputBuffer.Get(), m_InputUploadBuffer.Get(), 0, 0, 1, &subResourceData);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_InputBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    pComList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(pPipeline->commandList->Close());
    ID3D12CommandList* cmdsLists[] = { pPipeline->commandList.Get() };
    pPipeline->commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    pPipeline->Flush();
}
