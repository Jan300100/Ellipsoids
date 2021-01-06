#pragma once
#include "Stage.h"
#include <vector>

class DX12;
class QuadricRenderer;
class QuadricGeometry;
namespace Stage
{
	class GeometryProcessing : public Stage
	{
	public:
		GeometryProcessing(DX12* pDX12);
		bool Execute(QuadricRenderer* pRenderer, QuadricGeometry* pGeometry) const;
		virtual void Init(QuadricRenderer* pRenderer) override;
	}; 
}