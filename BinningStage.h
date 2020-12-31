#pragma once
#include <wrl.h>
#include <d3d12.h>

class QuadricMesh;
class DX12;
namespace Stage
{
	class Binning
	{
		Microsoft::WRL::ComPtr<ID3DBlob> m_Shader;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
		DX12* m_pDX12;
	public:
		Binning(DX12* pDX12);
		void Execute(
			ID3D12Resource* appDataBuffer,
			ID3D12Resource* screenTileBuffer,
			ID3D12Resource* tileBuffer,
			ID3D12Resource* quadricIndexBuffer,
			const QuadricMesh& mesh
		);
	};
}