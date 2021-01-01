#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Stage.h"

class DX12;
class QuadricMesh;
namespace Stage
{
	class Projection : public Stage
	{
	public:
		Projection(DX12* pDX12);
		void Execute(QuadricRenderer* pRenderer, QuadricMesh* pMesh) const;
	};
}