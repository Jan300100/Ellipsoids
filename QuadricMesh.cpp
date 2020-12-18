#include "QuadricMesh.h"
#include "d3dx12.h"
#include "Helpers.h"
#include "DX12.h"
#include <iostream>

using namespace DirectX;

void QuadricMesh::UpdateMeshData()
{
    XMMATRIX tr = XMMatrixAffineTransformation(XMLoadFloat3(&m_Transform.scale), XMVectorZero()
        , XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_Transform.rotation))
        , XMLoadFloat3(&m_Transform.position));
    tr = XMMatrixInverse(nullptr, tr);

    BYTE* mapped = nullptr;
    m_MeshDataBuffer->Map(0, nullptr,
        reinterpret_cast<void**>(&mapped));
    memcpy(mapped, &tr, sizeof(XMMATRIX));
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
    
    // Create the buffer that will be a UAV with outputquadrics
    byteSize = sizeof(OutQuadric) * m_Quadrics.size();
    properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&m_OutputProjectedBuffer)));


    UpdateBuffers(pDX12);


    //create transform data
    //Constant Buffer
    properties = { CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
    desc = { CD3DX12_RESOURCE_DESC::Buffer((sizeof(XMMATRIX) + 255) & ~255) };
    pDX12->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_MeshDataBuffer));

    //set data
    UpdateMeshData();
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