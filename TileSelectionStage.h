#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Stage.h"

class DX12;

struct Counters
{
	unsigned int tileCtr;
	unsigned int quadricCtr;
};

namespace Stage
{
	class TileSelection : public Stage
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> m_CountersResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_CountersUploadResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_CountersReadbackResource;
	public:
		TileSelection(DX12* pDX12);
		void Execute(QuadricRenderer* pRenderer, QuadricMesh* pMesh = nullptr) const;
	};
}