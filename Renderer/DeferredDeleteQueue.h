#pragma once
#include <vector>

struct ID3D12Resource;

class DeferredDeleteQueue
{
	struct PendingDeleteResource
	{
		ID3D12Resource* resource;
		uint32_t pendingFrames;

	};
	std::vector<PendingDeleteResource> m_DeferredDeleteQueue;
	uint32_t m_Hysteresis;
public:
	DeferredDeleteQueue(uint32_t hysteresis);
	~DeferredDeleteQueue();

	void QueueForDelete(ID3D12Resource* resource);
	void Step();
};