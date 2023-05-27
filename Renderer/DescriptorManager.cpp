#include "DescriptorManager.h"

DescriptorManager DescriptorManager::s_Instance;

DescriptorManager::~DescriptorManager()
{
	if (m_pDescriptorHeap)
	{
		m_pDescriptorHeap->Release();
	}

	if (m_pDescriptorHeapSV)
	{
		m_pDescriptorHeapSV->Release();
	}
}

DescriptorManager* DescriptorManager::Instance()
{
	return &s_Instance;
}
