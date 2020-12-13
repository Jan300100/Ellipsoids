#pragma once
#include "Quadric.h"


class MultiQuadric
{
	std::vector<Quadric> m_Quadrics; //data cpu
	ComPtr<ID3D12Resource> m_QuadricBuffer; //not projected data on the gpu
	ComPtr<ID3D12Resource> m_ProjectedQuadricBuffer; //projected data on the gpu ()
public:
	MultiQuadric();
	void AddQuadric(const Quadric& q);
	inline ComPtr<ID3D12Resource> GetProjectedBuffer() const { return m_ProjectedQuadricBuffer; }
};