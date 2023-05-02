#include "DeferredDeleteQueue.h"
#include <d3d12.h>

DeferredDeleteQueue::DeferredDeleteQueue(uint32_t hysteresis)
	:m_Hysteresis{hysteresis}
{
}

DeferredDeleteQueue::~DeferredDeleteQueue()
{
	for (size_t i = 0; i < m_DeferredDeleteQueue.size(); i++)
		m_DeferredDeleteQueue[i].resource->Release();
}

void DeferredDeleteQueue::QueueForDelete(ID3D12Resource* resource)
{
	PendingDeleteResource r;
	r.pendingFrames = 0;
	r.resource = resource;
	m_DeferredDeleteQueue.push_back(r);
}

void DeferredDeleteQueue::Step()
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