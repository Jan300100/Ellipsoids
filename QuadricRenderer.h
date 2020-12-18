#pragma once
#include <vector>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#include <imgui.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include "Quadric.h"

class QuadricMesh;

struct FrameData
{
	DirectX::XMMATRIX viewProjInv;
	DirectX::XMMATRIX viewInv;
	DirectX::XMMATRIX projInv;
	DirectX::XMFLOAT4 windowSize;
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
	enum class Stage {Projection, Rasterization, NumStages};

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature[int(Stage::NumStages)];
	Microsoft::WRL::ComPtr<ID3DBlob> m_Shader[int(Stage::NumStages)];
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso[int(Stage::NumStages)];
	//DATA
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeapShaderVisible;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_OutputTexture;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthTexture;
	FrameData m_FrameData;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_FrameDataBuffer; //general data (for both stages?)

	//Initialization
	void InitResources();
	void InitProjectionStage();
	void InitRasterizationStage();

	//RENDER
	void ProjectionStage();
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