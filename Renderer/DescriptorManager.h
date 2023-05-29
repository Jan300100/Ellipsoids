#pragma once
#include <d3d12.h>
#include "GPUResource.h"
#include "GPUBuffer.h"
#include "GPUTexture2D.h"
#include <vector>

class DescriptorManager
{
private:
	DescriptorManager() = default;
	~DescriptorManager();
	static DescriptorManager s_Instance;
public:
	static DescriptorManager* Instance();

	void Initialize(ID3D12Device* pDevice);
	
	GPUResource::Descriptor CreateUAV(const GPUTexture2D& resource);
	GPUResource::Descriptor CreateUAV(const GPUBuffer& resource);
	GPUResource::Descriptor CreateSRV(const GPUTexture2D& resource);
	GPUResource::Descriptor CreateSRV(const GPUBuffer& resource);
	GPUResource::Descriptor CreateCBV(const GPUBuffer& resource);

	void Free(const GPUResource::Descriptor& data);

	ID3D12DescriptorHeap* GetShaderVisibleHeap() const { return m_pDescriptorHeapSV; }
private:
	GPUResource::Descriptor Allocate();

	bool m_Initialized = false;

	ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* m_pDescriptorHeapSV = nullptr;

	ID3D12Device* m_pDevice = nullptr;
	uint32_t m_IncrementSize = 0;

	uint32_t m_NumDescriptors = 0;

	GPUResource::Descriptor m_Next = {};
	std::vector<GPUResource::Descriptor> m_FreeList = {};
};