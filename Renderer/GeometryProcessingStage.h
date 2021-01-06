#pragma once
#include "Stage.h"

class QuadricRenderer;
class QuadricGeometry;
namespace Stage
{
	class GeometryProcessing : public Stage
	{
	public:
		GeometryProcessing();
		bool Execute(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList, QuadricGeometry* pGeometry) const;
		virtual void Init(QuadricRenderer* pRenderer) override;
	}; 
}