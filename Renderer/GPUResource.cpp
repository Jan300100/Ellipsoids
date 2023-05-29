#include "GPUResource.h"
#include "DeferredDeleteQueue.h"
#include "DescriptorManager.h"
#include "Helpers.h"

#include <array>

CD3DX12_RESOURCE_BARRIER GPUResource::TransitionResource(D3D12_RESOURCE_STATES newState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_Resource,
		m_CurrentState, newState);
				
	m_CurrentState = newState;

	return barrier;
}

GPUResource::Descriptor GPUResource::GetUAV()
{
	if (m_UavDescriptor.isActive == false)
	{
		m_UavDescriptor = DescriptorManager::Instance()->CreateUAV(*this);
	}

	return m_UavDescriptor;
}

GPUResource::Descriptor GPUResource::GetSRV()
{
	if (m_SrvDescriptor.isActive == false)
	{
		m_SrvDescriptor = DescriptorManager::Instance()->CreateSRV(*this);
	}

	return m_SrvDescriptor;
}

void* GPUResource::Map()
{
	void* mapped = nullptr;
	if (m_UploadResource)
	{
		HRESULT hr = m_UploadResource->Map(0, nullptr, &mapped);
		ThrowIfFailed(hr);
	}

	return mapped;
}

void GPUResource::Unmap(ID3D12GraphicsCommandList* pComList)
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

DXGI_FORMAT GPUResource::GetFormat() const
{
	if (m_Type == Type::Texture2D)
	{
		return m_Texture2DParams.format;
	}
	return DXGI_FORMAT_UNKNOWN;
}

GPUResource::Type GPUResource::GetType() const
{
	return m_Type;
}

GPUResource::GPUResource()
	: m_Resource{ nullptr }
	, m_UploadResource{ nullptr }
	, m_BufferParams{}
	, m_Texture2DParams{}
	, m_CurrentState{}
	, m_Type{}
	, m_UavDescriptor{}
	, m_SrvDescriptor{}
{
}

GPUResource::GPUResource(ID3D12Device* pDevice, const BufferParams& params)
	: m_Resource{ nullptr }
	, m_UploadResource{ nullptr }
	, m_BufferParams{ params }
	, m_Texture2DParams{}
	, m_CurrentState{}
	, m_Type{Type::Buffer}
	, m_UavDescriptor{}
	, m_SrvDescriptor{}
{
	CD3DX12_HEAP_PROPERTIES properties{params.heapType};
	CD3DX12_RESOURCE_DESC desc = { CD3DX12_RESOURCE_DESC::Buffer(params.elementSize * params.numElements) };
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
		properties = CD3DX12_HEAP_PROPERTIES{D3D12_HEAP_TYPE_UPLOAD};

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
}

GPUResource::GPUResource(ID3D12Device* pDevice, const Texture2DParams& params)
	: m_Resource{ nullptr }
	, m_UploadResource{ nullptr }
	, m_BufferParams{}
	, m_Texture2DParams{ params }
	, m_CurrentState{ D3D12_RESOURCE_STATE_COMMON }
	, m_Type{ Type::Texture2D }
	, m_UavDescriptor{}
	, m_SrvDescriptor{}
{
	CD3DX12_HEAP_PROPERTIES properties{ D3D12_HEAP_TYPE_DEFAULT };
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(params.format, params.width, params.height);
	desc.MipLevels = params.numMips;

	if (params.allowUAV)
	{
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		m_CurrentState,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));
}

GPUResource::GPUResource(GPUResource&& other) noexcept
	:m_Resource{ other.m_Resource }
	, m_UploadResource{other.m_UploadResource}
	, m_BufferParams{other.m_BufferParams}
	, m_Texture2DParams{other.m_Texture2DParams}
	, m_CurrentState{other.m_CurrentState}
	, m_Type{other.m_Type}
	, m_UavDescriptor{ other.m_UavDescriptor}
	, m_SrvDescriptor{ other.m_SrvDescriptor}
{
	other.m_Resource = nullptr;
	other.m_UploadResource = nullptr;
	other.m_BufferParams = {};
	other.m_Texture2DParams = {};
	other.m_CurrentState = {};
	other.m_Type = {};
	other.m_UavDescriptor = {};
	other.m_SrvDescriptor = {};
}

GPUResource& GPUResource::operator=(GPUResource&& other) noexcept
{
	DescriptorManager::Instance()->Free(m_UavDescriptor);
	DescriptorManager::Instance()->Free(m_SrvDescriptor);

	if (m_Resource != m_UploadResource)
	{
		DeferredDeleteQueue::Instance()->QueueForDelete(m_UploadResource);
	}
	DeferredDeleteQueue::Instance()->QueueForDelete(m_Resource);

	//

	m_Resource = other.m_Resource;
	m_BufferParams = other.m_BufferParams;
	m_Texture2DParams = other.m_Texture2DParams;
	m_UploadResource = other.m_UploadResource;
	m_CurrentState = other.m_CurrentState;
	m_Type = other.m_Type;
	m_UavDescriptor = other.m_UavDescriptor;
	m_SrvDescriptor = other.m_SrvDescriptor;

	other.m_Resource = nullptr;
	other.m_UploadResource = nullptr;
	other.m_BufferParams = {};
	other.m_Texture2DParams = {};
	other.m_CurrentState = {};
	other.m_Type = {};
	other.m_UavDescriptor = {};
	other.m_SrvDescriptor = {};

	return *this;
}

GPUResource::~GPUResource()
{
	DescriptorManager::Instance()->Free(m_UavDescriptor);
	DescriptorManager::Instance()->Free(m_SrvDescriptor);

	if (m_Resource != m_UploadResource)
	{
		DeferredDeleteQueue::Instance()->QueueForDelete(m_UploadResource);
	}
	DeferredDeleteQueue::Instance()->QueueForDelete(m_Resource);
}
