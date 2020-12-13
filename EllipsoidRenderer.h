#pragma once
#include "Ellipsoid.h" 
#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

struct FrameData
{
	DirectX::XMFLOAT4 windowSize;
	DirectX::XMFLOAT4 lightDirection;
};



class DX12;
class Camera;
class EllipsoidRenderer
{
private:
	DX12* m_pDX12;
	Camera* m_pCamera;
	//
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob> m_Shader;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso;
	//DATA
	ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	ComPtr<ID3D12Resource> m_OutputTexture;
	ComPtr<ID3D12Resource> m_InputEllipsoidBuffer; //ellipsoid data
	ComPtr<ID3D12Resource> m_InputDataBuffer; //general data
public:
	EllipsoidRenderer(DX12* pDX12, Camera* pCamera);
	void SetCamera(Camera* pCamera) { m_pCamera = pCamera; }

	void RenderStart();
	void RenderFinish();
	void Render(const Ellipsoid& e);
};