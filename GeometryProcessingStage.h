#pragma once
#include "Stage.h"
#include <vector>

class DX12;
class QuadricRenderer;
class QuadricMesh;
namespace Stage
{
	class GeometryProcessing : public Stage
	{
	public:
		GeometryProcessing(DX12* pDX12);
		bool Execute(QuadricRenderer* pRenderer, QuadricMesh* pMesh) const;
		virtual void Init(QuadricRenderer* pRenderer) override;
	}; 
}