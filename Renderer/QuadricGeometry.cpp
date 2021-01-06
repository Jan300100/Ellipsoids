#include "QuadricGeometry.h"
#include "d3dx12.h"
#include "Helpers.h"
#include <iostream>

using namespace DirectX;

UINT QuadricGeometry::UpdateTransforms()
{
    BYTE* mapped = nullptr;
    m_MeshDataBuffer->Map(0, nullptr,
        reinterpret_cast<void**>(&mapped));
    UINT amount = (UINT)m_Transforms.size();

    if (amount > m_MaxInstances)
    {
        std::cerr << "Too many instances";
        amount = m_MaxInstances;
    }

    memcpy(mapped, m_Transforms.data(), sizeof(XMMATRIX) * amount);
    if (m_MeshDataBuffer != nullptr)
        m_MeshDataBuffer->Unmap(0, nullptr);

    m_Transforms.clear();
    return amount;
}

QuadricGeometry::QuadricGeometry(ID3D12Device2* pDevice, ID3D12GraphicsCommandList* pComList, const std::vector<Quadric>& quadrics, UINT maxInstances)
    :m_Quadrics{ quadrics }, m_Transforms{}, m_MaxInstances{(maxInstances == 0) ? 1 : maxInstances}
{
    size_t byteSize = sizeof(Quadric) * m_Quadrics.size();
    CD3DX12_HEAP_PROPERTIES properties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
    CD3DX12_RESOURCE_DESC desc{ CD3DX12_RESOURCE_DESC::Buffer(byteSize) };

    // Create the actual default buffer resource.
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_InputBuffer.GetAddressOf())));

    properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    ThrowIfFailed(pDevice->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_InputUploadBuffer.GetAddressOf())));
    
    UpdateBuffers(pComList);

    //create transform data
    //Constant Buffer
    properties = { CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
    desc = { CD3DX12_RESOURCE_DESC::Buffer(sizeof(DirectX::XMMATRIX) * m_MaxInstances)};
    pDevice->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_MeshDataBuffer));

    //set data
    UpdateTransforms();
}

void QuadricGeometry::UpdateBuffers(ID3D12GraphicsCommandList* pComList)
{
    size_t byteSize = sizeof(Quadric) * m_Quadrics.size();

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
}
