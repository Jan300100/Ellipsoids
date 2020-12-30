#include "QuadricMesh.h"
#include "d3dx12.h"
#include "Helpers.h"
#include "DX12.h"
#include <iostream>

using namespace DirectX;
ID3D12Resource* QuadricMesh::m_ShaderOutputUploadBuffer{nullptr};
unsigned int QuadricMesh::m_NrMeshes{};

void QuadricMesh::UpdateMeshData()
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
    m_NrMeshes++;

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


    //create shaderoutputBuffer
    byteSize = sizeof(ShaderOutput);
    properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_ShaderOutputBuffer)));
    m_ShaderOutputBuffer->SetName(LPCWSTR(L"ShaderOutputBuffer"));

    //create shaderoutputBuffer : readback
    properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK);
    desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize,D3D12_RESOURCE_FLAG_NONE);
    ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_ShaderOutputReadbackBuffer)));
    m_ShaderOutputReadbackBuffer->SetName(LPCWSTR(L"ShaderOutputReadBackBuffer"));

    if (m_ShaderOutputUploadBuffer == nullptr)
    {
        properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
            &properties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_ShaderOutputUploadBuffer)));

        ShaderOutput initial{ 0 };
        ShaderOutput* mapped = nullptr;
        m_ShaderOutputUploadBuffer->Map(0, nullptr,
            reinterpret_cast<void**>(&mapped));
        memcpy(mapped, &initial, sizeof(ShaderOutput));
        if (m_ShaderOutputUploadBuffer != nullptr)
            m_ShaderOutputUploadBuffer->Unmap(0, nullptr);
    }
}

QuadricMesh::~QuadricMesh()
{
    if (--m_NrMeshes == 0)
    {
        m_ShaderOutputUploadBuffer->Release();
        m_ShaderOutputUploadBuffer = nullptr;
    }
}

ShaderOutput QuadricMesh::GetShaderOutput()
{
    ShaderOutput* mapped = nullptr;
    m_ShaderOutputReadbackBuffer->Map(0, nullptr,
        reinterpret_cast<void**>(&mapped));
    m_Output = *mapped;
    if (m_ShaderOutputReadbackBuffer != nullptr)
        m_ShaderOutputReadbackBuffer->Unmap(0, nullptr);
    return m_Output;
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
