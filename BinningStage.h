#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Stage.h"

class QuadricMesh;
class DX12;
namespace Stage
{
	class Binning : public Stage
	{
	public:
		Binning(DX12* pDX12);
		void Execute(QuadricRenderer* pRenderer, QuadricMesh* pMesh) const;
	};
}