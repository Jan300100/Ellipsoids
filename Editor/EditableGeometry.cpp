#include "EditableGeometry.h"
#include "QuadricGeometry.h"
#include <dxgi1_6.h>

void EditableGeometry::UpdateGeometry(ID3D12Device2* pDevice, ID3D12GraphicsCommandList* pComList)
{
	std::vector<Quadric> q{};
	for (EditQuadric& eq : quadrics)
	{
		q.push_back(eq.ToQuadric());
	}

	pGeometry->Init(pDevice, pComList, q, pGeometry->GetMaxInstances());
}
