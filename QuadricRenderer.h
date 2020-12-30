#pragma once
#include <vector>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#include <imgui.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include "ProjectionStage.h"

#include "Quadric.h"

class QuadricMesh;

struct AppData
{
	DirectX::XMMATRIX viewProjInv;
	DirectX::XMMATRIX viewInv;
	DirectX::XMMATRIX projInv;
	DirectX::XMUINT4 windowSize;
	DirectX::XMFLOAT4 lightDirection;
};

class DX12;
class Camera;
class QuadricRenderer
{
private:
	DX12* m_pDX12;
	Camera* m_pCamera;
	//

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob> m_Shader;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso;
	//DATA
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeapShaderVisible;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_OutputTexture;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthTexture;
	AppData m_AppData;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_AppDataBuffer; //general data (for both stages?)
	//Initialization
	void InitResources();
	void InitRasterizationStage();

	//RENDER
	Stage::Projection m_ProjStage;
	void RasterizationStage();
	void CopyToBackBuffer();
	
	std::vector<QuadricMesh*> m_ToRender;

public:
	OutQuadric Project(const Quadric& e);
	QuadricRenderer(DX12* pDX12, Camera* pCamera);
	void SetCamera(Camera* pCamera) { m_pCamera = pCamera; }

	void Render();
	void Render(QuadricMesh* pMesh);
};