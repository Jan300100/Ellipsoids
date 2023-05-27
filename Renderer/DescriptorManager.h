#pragma once
#include <d3d12.h>

// called by the GPU Resource, not by QuadricRenderer Directly (?)
class DescriptorManager
{
private:
	DescriptorManager() = default;
	~DescriptorManager();
	static DescriptorManager s_Instance;

private:
	ID3D12DescriptorHeap* m_DescriptorHeap;
	ID3D12DescriptorHeap* m_DescriptorHeapSV;
public:
	static DescriptorManager* Instance();

	// move descriptorheaps here.
	// let GPUResource add descriptors
	// ask uav/srv index using a gpuresource as key.
	// creates descriptor if not here already and returns the index.
};