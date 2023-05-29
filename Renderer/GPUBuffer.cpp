#include "GPUBuffer.h"
#include "Helpers.h"
#include "DescriptorManager.h"
#include <array>

void* GPUBuffer::Map()
{
	void* mapped = nullptr;
	if (m_UploadResource)
	{
		HRESULT hr = m_UploadResource->Map(0, nullptr, &mapped);
		ThrowIfFailed(hr);
	}

	return mapped;
}

void GPUBuffer::Unmap(ID3D12GraphicsCommandList* pComList)
{
	if (m_UploadResource)
	{
		m_UploadResource->Unmap(0, nullptr);

		if (m_UploadResource != m_Resource)
		{
			D3D12_RESOURCE_STATES originalState = m_CurrentState;

			std::array<CD3DX12_RESOURCE_BARRIER, 2> barriers{};
			barriers[0] = TransitionResource(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_UploadResource,
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
			pComList->ResourceBarrier(2, barriers.data());

			pComList->CopyResource(m_Resource, m_UploadResource);

			barriers[0] = TransitionResource(originalState);
			barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_UploadResource,
				D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
			pComList->ResourceBarrier(2, barriers.data());
		}
	}
}

GPUBuffer::GPUBuffer(ID3D12Device* pDevice, const Params& params)
	:GPUResource{}
	, m_Params{ params }
	, m_CbvDescriptor{}
	, m_Size{ (params.elementSize * params.numElements + 255) & ~255 }
{
	m_Type = GPUResource::Type::Buffer;

	CD3DX12_HEAP_PROPERTIES properties{ params.heapType };
	
	CD3DX12_RESOURCE_DESC desc = { CD3DX12_RESOURCE_DESC::Buffer(m_Size) };
	if (params.allowUAV)
	{
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	m_CurrentState = params.heapType == D3D12_HEAP_TYPE_UPLOAD ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		m_CurrentState,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));

	if (params.heapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		m_UploadResource = m_Resource;
	}
	else
	{
		// create separate uploadresource so we can upload data
		properties = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_UPLOAD };

		if (params.allowUAV)
		{
			desc.Flags ^= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		ThrowIfFailed(pDevice->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_UploadResource)));
	}

	if (m_Params.allowUAV)
	{
		m_UavDescriptor = DescriptorManager::Instance()->CreateUAV(*this);
	}
	m_SrvDescriptor = DescriptorManager::Instance()->CreateSRV(*this);

	if (m_Size <= 64 * 1024)
	{
		m_CbvDescriptor = DescriptorManager::Instance()->CreateCBV(*this);
	}

}

GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
	:GPUResource{std::move(other)}
	, m_Params{other.m_Params}
	, m_CbvDescriptor{other.m_CbvDescriptor}
	, m_Size{other.m_Size}
{
	other.m_Params = {};
	other.m_CbvDescriptor = {};
	other.m_Size = {};
}

GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept
{
	GPUResource::operator=(std::move(other));
	DescriptorManager::Instance()->Free(m_CbvDescriptor);

	m_Params = other.m_Params;
	m_CbvDescriptor = other.m_CbvDescriptor;
	m_Size = other.m_Size;

	other.m_Params = {};
	other.m_CbvDescriptor = {};
	other.m_Size = {};

	return *this;
}

GPUBuffer::~GPUBuffer()
{
	DescriptorManager::Instance()->Free(m_CbvDescriptor);
}
