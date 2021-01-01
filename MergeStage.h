#pragma once
#include "Stage.h"

class DX12;
class QuadricRenderer;
namespace Stage
{
	class Merge : public Stage
	{
	public:
		Merge(DX12* pDX12);
		void Execute(QuadricRenderer* pRenderer) const;
	};
}