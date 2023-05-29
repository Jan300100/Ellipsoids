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

GPUResource::Type GPUResource::GetType() const
{
	return m_Type;
}


GPUResource::GPUResource(GPUResource&& other) noexcept
	:m_Resource{ other.m_Resource }
	, m_UploadResource{other.m_UploadResource}
	, m_CurrentState{other.m_CurrentState}
	, m_Type{other.m_Type}
	, m_UavDescriptor{ other.m_UavDescriptor}
	, m_SrvDescriptor{ other.m_SrvDescriptor }
{
	other.m_Resource = nullptr;
	other.m_UploadResource = nullptr;
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

	m_Resource = other.m_Resource;
	m_UploadResource = other.m_UploadResource;
	m_CurrentState = other.m_CurrentState;
	m_Type = other.m_Type;
	m_UavDescriptor = other.m_UavDescriptor;
	m_SrvDescriptor = other.m_SrvDescriptor;

	other.m_Resource = nullptr;
	other.m_UploadResource = nullptr;
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
