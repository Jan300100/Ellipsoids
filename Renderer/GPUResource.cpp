#include "GPUResource.h"
#include "DeferredDeleteQueue.h"
#include "Helpers.h"

GPUResource::GPUResource()
	:m_DeleteQueue{ nullptr }
	, m_Resource{ nullptr }
{
}

GPUResource::GPUResource(ID3D12Device* pDevice, DeferredDeleteQueue* pDeleteQueue, Params params)
	:m_DeleteQueue{ pDeleteQueue }
	, m_Resource{ nullptr }
	, m_Params{params}
{
	// singleFrame resource
	CD3DX12_HEAP_PROPERTIES properties = { CD3DX12_HEAP_PROPERTIES(params.heapType) };
	CD3DX12_RESOURCE_DESC desc = { CD3DX12_RESOURCE_DESC::Buffer(params.size)};
	ThrowIfFailed(pDevice->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));
}

GPUResource::GPUResource(GPUResource&& other)
	:m_Resource{ other.m_Resource }
	, m_DeleteQueue{ other.m_DeleteQueue }
{
	other.m_DeleteQueue = nullptr;
	other.m_Resource = nullptr;
}

GPUResource& GPUResource::operator=(GPUResource&& other)
{
	m_DeleteQueue->QueueForDelete(m_Resource);

	m_DeleteQueue = other.m_DeleteQueue;
	m_Resource = other.m_Resource;

	other.m_DeleteQueue = nullptr;
	other.m_Resource = nullptr;

	return *this;
}

GPUResource::~GPUResource()
{
	m_DeleteQueue->QueueForDelete(m_Resource);
}
