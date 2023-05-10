#include "QuadricGeometry.h"
#include "QuadricRenderer.h"
#include "d3dx12.h"
#include "Helpers.h"
#include <iostream>

using namespace DirectX;

void QuadricGeometry::UpdateTransforms(ID3D12GraphicsCommandList* pComList, QuadricRenderer* pRenderer)
{    
    if (m_Initialized && m_Transforms.size() > 0)
    {
        if (m_Transforms.size() > m_NumInstances)
        {
            m_NumInstances = m_Transforms.size();
            RecreateMeshBuffer(pRenderer);
        }

        // create temp resource and queue for delete
        ID3D12Resource* tempResource;

        // singleframe upload resource
        CD3DX12_HEAP_PROPERTIES properties = { CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
        CD3DX12_RESOURCE_DESC desc = { CD3DX12_RESOURCE_DESC::Buffer(sizeof(DirectX::XMMATRIX) * (UINT)m_Transforms.size()) };
        pRenderer->GetDevice()->CreateCommittedResource(
            &properties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&tempResource));
        pRenderer->GetDeferredDeleteQueue()->QueueForDelete(tempResource);

        BYTE* mapped = nullptr;
        tempResource->Map(0, nullptr,
            reinterpret_cast<void**>(&mapped));

        memcpy(mapped, m_Transforms.data(), sizeof(XMMATRIX) * m_Transforms.size());
        if (tempResource != nullptr)
            tempResource->Unmap(0, nullptr);


        // queue copy
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshDataBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        pComList->ResourceBarrier(1, &barrier);

        pComList->CopyResource(m_MeshDataBuffer, tempResource);

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshDataBuffer,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
        pComList->ResourceBarrier(1, &barrier);
        m_Transforms.clear();
    }
}

void QuadricGeometry::Init(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList, const std::vector<Quadric>& quadrics)
{
    m_Quadrics = quadrics;

    size_t byteSize = sizeof(Quadric) * m_Quadrics.size();
    CD3DX12_HEAP_PROPERTIES properties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
    CD3DX12_RESOURCE_DESC desc{ CD3DX12_RESOURCE_DESC::Buffer(byteSize) };

    // Create the actual default buffer resource.
    ThrowIfFailed(pRenderer->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_InputBuffer.GetAddressOf())));

    properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    ThrowIfFailed(pRenderer->GetDevice()->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_InputUploadBuffer.GetAddressOf())));

    byteSize = sizeof(Quadric) * m_Quadrics.size();

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = m_Quadrics.data();
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_InputBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    pComList->ResourceBarrier(1, &barrier);

    UpdateSubresources<1>(pComList, m_InputBuffer.Get(), m_InputUploadBuffer.Get(), 0, 0, 1, &subResourceData);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_InputBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    pComList->ResourceBarrier(1, &barrier);

    RecreateMeshBuffer(pRenderer);

    m_Initialized = true;
}

void QuadricGeometry::RecreateMeshBuffer(QuadricRenderer* pRenderer)
{
    if (m_NumInstances > 0)
    {
        if (m_MeshDataBuffer != nullptr)
        {
            pRenderer->GetDeferredDeleteQueue()->QueueForDelete(m_MeshDataBuffer);
        }

        CD3DX12_HEAP_PROPERTIES properties = { CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
        CD3DX12_RESOURCE_DESC desc = { CD3DX12_RESOURCE_DESC::Buffer(sizeof(DirectX::XMMATRIX) * m_NumInstances) };
        pRenderer->GetDevice()->CreateCommittedResource(
            &properties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_MeshDataBuffer));
    }
}

QuadricGeometry::QuadricGeometry(const std::string& name)
    :m_Quadrics{ }, m_Transforms{}, m_Name{name}
{
}

QuadricGeometry::~QuadricGeometry()
{
    if (m_MeshDataBuffer)
    {
        m_MeshDataBuffer->Release();
    }
}
