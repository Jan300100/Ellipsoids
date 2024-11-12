#include "QuadricGeometry.h"
#include "QuadricRenderer.h"
#include "d3dx12.h"
#include "Helpers.h"
#include <iostream>

using namespace DirectX;

void QuadricGeometry::UpdateTransforms(ID3D12GraphicsCommandList* pComList, QuadricRenderer* pRenderer)
{    
    if (m_Initialized)
    {
        if (m_Transforms.size() > m_NumInstances)
        {
            m_NumInstances = m_Transforms.size();
            RecreateInstanceBuffer(pRenderer);
        }

        void* mapped = m_InstanceBuffer.Map();
        memcpy(mapped, m_Transforms.data(), sizeof(XMMATRIX) * m_Transforms.size());
        m_InstanceBuffer.Unmap(pComList);

        m_DrawData.numQuadrics = (UINT)m_Quadrics.size();
        m_DrawData.numInstances = (UINT)m_Transforms.size();
        m_DrawData.instanceBufferIdx = m_InstanceBuffer.GetSRV().indexSV;
        m_DrawData.quadricBufferIdx = m_InputBuffer.GetSRV().indexSV;

        m_Transforms.clear();
    }
}

void QuadricGeometry::Init(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList, const std::vector<Quadric>& quadrics)
{
    m_Quadrics = quadrics;

    GPUBuffer::Params params{};
    params.numElements = (UINT)m_Quadrics.size();
    params.elementSize = sizeof(Quadric);
    params.heapType = D3D12_HEAP_TYPE_DEFAULT;

    m_InputBuffer = GPUBuffer{ pRenderer->GetDevice(), params};

    void* mapped = m_InputBuffer.Map();
    memcpy(mapped, m_Quadrics.data(), params.elementSize * params.numElements);
    m_InputBuffer.Unmap(pComList);

    RecreateInstanceBuffer(pRenderer);

    m_Initialized = true;
}

void QuadricGeometry::RecreateInstanceBuffer(QuadricRenderer* pRenderer)
{
    if (m_NumInstances > 0)
    {
        GPUBuffer::Params params{};
        params.numElements = static_cast<uint32_t>(m_NumInstances);
        params.elementSize = static_cast<uint32_t>(sizeof(DirectX::XMMATRIX));
        params.heapType = D3D12_HEAP_TYPE_DEFAULT;

        m_InstanceBuffer = GPUBuffer(pRenderer->GetDevice(), params);
    }
}

QuadricGeometry::QuadricGeometry(const std::string& name)
    :m_Quadrics{ }, m_Transforms{}, m_Name{ name }, m_NumInstances{}
{
}
