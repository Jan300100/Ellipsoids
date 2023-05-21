#pragma once
#include "d3dx12.h"

class DeferredDeleteQueue;

class GPUResource
{
public:
	struct Params {
		size_t size;
		D3D12_HEAP_TYPE heapType;
	};

	GPUResource();
	GPUResource(ID3D12Device* pDevice, Params params);
	GPUResource(GPUResource&&);
	GPUResource(const GPUResource&) = delete;
	GPUResource& operator=(GPUResource&&);
	GPUResource& operator=(const GPUResource&) = delete;
	~GPUResource();


	ID3D12Resource* Get() const { return m_Resource; }
private:
	ID3D12Resource* m_Resource;
	Params m_Params;
};