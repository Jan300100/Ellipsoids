#pragma once
#include <vector>

struct ID3D12Resource;
struct ID3D12Device;

class DeferredDeleteQueue
{
	struct PendingDeleteResource
	{
		ID3D12Resource* resource;
		uint32_t pendingFrames;
	};

	std::vector<PendingDeleteResource> m_DeferredDeleteQueue;
	uint32_t m_Hysteresis;

	DeferredDeleteQueue() = default;
	~DeferredDeleteQueue();
	static DeferredDeleteQueue m_Instance;

public:
	static DeferredDeleteQueue* Instance();

	void SetHysteresis(uint32_t hysteresis);
	void QueueForDelete(ID3D12Resource* resource);
	void BeginFrame();
};
