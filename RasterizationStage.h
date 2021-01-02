#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Stage.h"

class DX12;
namespace Stage
{
	class Rasterization : public Stage
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	public:
		Rasterization(DX12* pDX12);
		virtual void Init(QuadricRenderer* pRenderer) override;
		void Execute(QuadricRenderer* pRenderer) const;
	};
}