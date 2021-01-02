#pragma once
#include "Stage.h"

class DX12;
class QuadricRenderer;
namespace Stage
{
	class Merge : public Stage
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	public:
		Merge(DX12* pDX12);
		void Execute(QuadricRenderer* pRenderer) const;
		virtual void Init(QuadricRenderer* pRenderer);
	};
}