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
            RecreateInstanceBuffer(pRenderer);
        }

        void* mapped = m_InstanceBuffer.Map();
        memcpy(mapped, m_Transforms.data(), sizeof(XMMATRIX) * m_Transforms.size());
        m_InstanceBuffer.Unmap(pComList);

        m_Transforms.clear();

        DrawData* data = static_cast<DrawData*>(m_DrawDataBuffer.Map());
        data->instanceBufferIdx = m_InstanceBuffer.GetSRV().indexSV;
        data->quadricBufferIdx = m_InputBuffer.GetSRV().indexSV;
        m_DrawDataBuffer.Unmap(pComList);
    }
}

void QuadricGeometry::Init(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList, const std::vector<Quadric>& quadrics)
{
    m_Quadrics = quadrics;

    GPUResource::BufferParams params{};
    params.numElements = (UINT)m_Quadrics.size();
    params.elementSize = sizeof(Quadric);
    params.heapType = D3D12_HEAP_TYPE_DEFAULT;

    m_InputBuffer = GPUResource{ pRenderer->GetDevice(), params};

    void* mapped = m_InputBuffer.Map();
    memcpy(mapped, m_Quadrics.data(), params.elementSize * params.numElements);
    m_InputBuffer.Unmap(pComList);

    RecreateInstanceBuffer(pRenderer);

    params.allowUAV = false;
    params.heapType = D3D12_HEAP_TYPE_DEFAULT;
    params.elementSize = sizeof(DrawData);
    params.numElements = 1;
    m_DrawDataBuffer = GPUResource{ pRenderer->GetDevice(), params };

    m_Initialized = true;
}

void QuadricGeometry::RecreateInstanceBuffer(QuadricRenderer* pRenderer)
{
    if (m_NumInstances > 0)
    {
        GPUResource::BufferParams params{};
        params.numElements = static_cast<uint32_t>(m_NumInstances);
        params.elementSize = static_cast<uint32_t>(sizeof(DirectX::XMMATRIX));
        params.heapType = D3D12_HEAP_TYPE_DEFAULT;

        m_InstanceBuffer = GPUResource(pRenderer->GetDevice(), params);
    }
}

QuadricGeometry::QuadricGeometry(const std::string& name)
    :m_Quadrics{ }, m_Transforms{}, m_Name{ name }, m_NumInstances{}
{
}
