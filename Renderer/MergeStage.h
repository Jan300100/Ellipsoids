#pragma once
#include "Stage.h"

class QuadricRenderer;
namespace Stage
{
	class Merge : public Stage
	{
	public:
		Merge();
		void Execute(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList) const;
		virtual void Init(QuadricRenderer* pRenderer);
	};
}