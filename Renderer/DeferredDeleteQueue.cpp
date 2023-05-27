#include "DeferredDeleteQueue.h"
#include <d3d12.h>

DeferredDeleteQueue DeferredDeleteQueue::s_Instance;

DeferredDeleteQueue::~DeferredDeleteQueue()
{
	for (size_t i = 0; i < m_DeferredDeleteQueue.size(); i++)
		m_DeferredDeleteQueue[i].resource->Release();
}

DeferredDeleteQueue* DeferredDeleteQueue::Instance()
{
	return &s_Instance;
}

void DeferredDeleteQueue::SetHysteresis(uint32_t hysteresis)
{
	m_Hysteresis = hysteresis;
}

void DeferredDeleteQueue::QueueForDelete(ID3D12Resource* resource)
{
	if (resource != nullptr)
	{
		PendingDeleteResource r;
		r.pendingFrames = 0;
		r.resource = resource;
		m_DeferredDeleteQueue.push_back(r);
	}
}

void DeferredDeleteQueue::BeginFrame()
{
	for (size_t i = 0; i < m_DeferredDeleteQueue.size(); i++)
	{
		PendingDeleteResource& r = m_DeferredDeleteQueue[i];
		if (r.pendingFrames > m_Hysteresis)
		{
			if (r.resource)
			{
				r.resource->Release();
				r.resource = nullptr;
			}

			m_DeferredDeleteQueue[i] = m_DeferredDeleteQueue.back();
			m_DeferredDeleteQueue.pop_back();
			i--;
		}
		else
		{
			r.pendingFrames++;
		}
	}
}
