#include "QuadricGeometry.h"
#include "QuadricRenderer.h"
#include "d3dx12.h"
#include "Helpers.h"
#include <iostream>

using namespace DirectX;

UINT QuadricGeometry::UpdateTransforms(ID3D12GraphicsCommandList* pComList, QuadricRenderer* pRenderer)
{
    if (!m_Initialized) return 0;

    // create temp resource and queue for delete :D
    ID3D12Resource* tempResource;

    // singleframe upload resource
    CD3DX12_HEAP_PROPERTIES properties = { CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
    CD3DX12_RESOURCE_DESC desc = { CD3DX12_RESOURCE_DESC::Buffer(sizeof(DirectX::XMMATRIX) * m_MaxInstances) };
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
    UINT amount = (UINT)m_Transforms.size();

    if (amount > m_MaxInstances)
    {
        throw std::wstring{ L"Too many instances" };
    }

    memcpy(mapped, m_Transforms.data(), sizeof(XMMATRIX) * amount);
    if (tempResource != nullptr)
        tempResource->Unmap(0, nullptr);

    // queue copy
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshDataBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    pComList->ResourceBarrier(1, &barrier);

    pComList->CopyResource(m_MeshDataBuffer.Get(), tempResource);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshDataBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    pComList->ResourceBarrier(1, &barrier);

    m_Transforms.clear();
    return amount;
}

void QuadricGeometry::Init(ID3D12Device2* pDevice, ID3D12GraphicsCommandList* pComList, const std::vector<Quadric>& quadrics, UINT maxInstances)
{
    m_Quadrics = quadrics;

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

    m_MaxInstances = (maxInstances == 0) ? 1 : maxInstances;
    //create transform data
    //Constant Buffer
    properties = { CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) };
    desc = { CD3DX12_RESOURCE_DESC::Buffer(sizeof(DirectX::XMMATRIX) * m_MaxInstances) };
    pDevice->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_MeshDataBuffer));

    m_Initialized = true;

}

QuadricGeometry::QuadricGeometry(const std::string& name)
    :m_Quadrics{ }, m_Transforms{}, m_Name{name}, m_MaxInstances{ 0 }
{
}