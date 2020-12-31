#pragma once
#include <wrl.h>
#include <d3d12.h>

class DX12;
class QuadricRenderer;
class QuadricMesh;
namespace Stage
{
	class Stage
	{
	protected:
		Microsoft::WRL::ComPtr<ID3DBlob> m_Shader;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
		DX12* m_pDX12;
	public:
		virtual ~Stage() = default;
		Stage(DX12* pDX12) : m_pDX12{ pDX12 } {}
		virtual void Execute(QuadricRenderer* pRenderer, QuadricMesh* pMesh) const = 0;
	};
}