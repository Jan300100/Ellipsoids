#pragma once

// called by the GPU Resource, not by QuadricRenderer Directly (?)
class DescriptorManager
{
private:
	DescriptorManager() = default;
	~DescriptorManager() = default;
	static DescriptorManager s_Instance;
public:
	static DescriptorManager* Instance();

	// move descriptorheaps here.
	// let GPUResource add descriptors
	// ask uav/srv index using a gpuresource as key.
	// creates descriptor if not here already and returns the index.
};