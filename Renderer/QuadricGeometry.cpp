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
        GPUResource::Params params;
        params.size = sizeof(DirectX::XMMATRIX) * (UINT)m_Transforms.size();
        params.heapType = D3D12_HEAP_TYPE_UPLOAD;
        GPUResource tempResource{ pRenderer->GetDevice(),params };

        BYTE* mapped = nullptr;
        tempResource.Get()->Map(0, nullptr,
            reinterpret_cast<void**>(&mapped));

        memcpy(mapped, m_Transforms.data(), sizeof(XMMATRIX) * m_Transforms.size());
        tempResource.Get()->Unmap(0, nullptr);

        // queue copy
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshDataBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        pComList->ResourceBarrier(1, &barrier);

        pComList->CopyResource(m_MeshDataBuffer.Get(), tempResource.Get());

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshDataBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
        pComList->ResourceBarrier(1, &barrier);
        m_Transforms.clear();
    }
}

void QuadricGeometry::Init(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList, const std::vector<Quadric>& quadrics)
{
    m_Quadrics = quadrics;

    GPUResource::Params params{};
    params.size = sizeof(Quadric) * m_Quadrics.size();
    params.heapType = D3D12_HEAP_TYPE_DEFAULT;

    m_InputBuffer = GPUResource{ pRenderer->GetDevice(), params};

    params.heapType = D3D12_HEAP_TYPE_UPLOAD;
    GPUResource temp{ pRenderer->GetDevice(), params };

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = m_Quadrics.data();
    subResourceData.RowPitch = params.size;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_InputBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    pComList->ResourceBarrier(1, &barrier);

    UpdateSubresources<1>(pComList, m_InputBuffer.Get(), temp.Get(), 0, 0, 1, &subResourceData);

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
        GPUResource::Params params{};
        params.size = sizeof(DirectX::XMMATRIX) * m_NumInstances;
        params.heapType = D3D12_HEAP_TYPE_DEFAULT;

        m_MeshDataBuffer = GPUResource(pRenderer->GetDevice(), params);
    }
}

QuadricGeometry::QuadricGeometry(const std::string& name)
    :m_Quadrics{ }, m_Transforms{}, m_Name{ name }, m_NumInstances{}
{
}
