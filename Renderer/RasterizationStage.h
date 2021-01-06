#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Stage.h"
#include <dxgi1_6.h>


namespace Stage
{
	class Rasterization : public Stage
	{
	public:
		Rasterization();
		virtual void Init(QuadricRenderer* pRenderer) override;
		void Execute(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList) const;
	};
}