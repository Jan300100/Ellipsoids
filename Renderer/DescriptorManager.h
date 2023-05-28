#pragma once
#include <d3d12.h>
#include "GPUResource.h"
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
	
	GPUResource::Descriptor CreateUAV(const GPUResource& resource);
	GPUResource::Descriptor CreateSRV(const GPUResource& resource);

	void Free(const GPUResource::Descriptor& data);

	ID3D12DescriptorHeap* GetShaderVisibleHeap() const { return m_pDescriptorHeapSV; }
private:
	D3D12_UAV_DIMENSION GetUAVViewDimension(GPUResource::Type type);;
	D3D12_SRV_DIMENSION GetSRVViewDimension(GPUResource::Type type);;

	bool m_Initialized = false;

	ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* m_pDescriptorHeapSV = nullptr;

	ID3D12Device* m_pDevice = nullptr;
	uint32_t m_IncrementSize = 0;

	uint32_t m_NumDescriptors = 0;

	GPUResource::Descriptor m_Next = {};
	std::vector<GPUResource::Descriptor> m_FreeList = {};
};