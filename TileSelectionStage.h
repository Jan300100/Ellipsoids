#pragma once
#include <d3d12.h>
#include <wrl.h>

class DX12;

struct Counters
{
	unsigned int tileCtr;
	unsigned int quadricCtr;
};

namespace Stage
{
	class TileSelection
	{
	private:
		Microsoft::WRL::ComPtr<ID3DBlob> m_Shader;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
		DX12* m_pDX12;
		//
		Microsoft::WRL::ComPtr<ID3D12Resource> m_CountersResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_CountersUploadResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_CountersReadbackResource;
	public:
		TileSelection(DX12* pDX12);
		void Execute(ID3D12Resource* appDataBuffer, ID3D12Resource* screenTileBuffer, ID3D12Resource* tileBuffer, unsigned int numScreenTiles);
	};
}