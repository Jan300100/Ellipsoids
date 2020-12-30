#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <vector>

class DX12;
class QuadricMesh;
namespace Stage
{
	class Projection
	{
	private:
		Microsoft::WRL::ComPtr<ID3DBlob> m_Shader;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
		DX12* m_pDX12;
	public:
		Projection(DX12* pDX12);
		void Project(Microsoft::WRL::ComPtr<ID3D12Resource> appDataBuffer, const std::vector<QuadricMesh*> meshes) const;
	};
}