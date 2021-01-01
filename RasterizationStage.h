#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Stage.h"

class DX12;
namespace Stage
{
	class Rasterization : public Stage
	{
	public:
		Rasterization(DX12* pDX12);
		void Execute(QuadricRenderer* pRenderer, QuadricMesh* pMesh) const;
	};
}