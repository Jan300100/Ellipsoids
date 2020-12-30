#pragma once
#include <vector>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#include <imgui.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include "ProjectionStage.h"
#include "Structs.h"

class QuadricMesh;
class DX12;
class Camera;
class QuadricRenderer
{
private:
	DX12* m_pDX12;
	Camera* m_pCamera;

	//DATA
	AppData m_AppData;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_AppDataBuffer; //general data (for both stages?)
	//TEXTURES
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeapShaderVisible;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_OutputTexture;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthTexture;

	//TODO:
	enum GBuffers : unsigned int
	{
		Depth = 0, Normals, Color, Amount
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_GBuffers[GBuffers::Amount]; //depth, normals, color
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScreenTileBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_TileBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_QuadricDistributionBuffer;

	
	//Initialization
	void InitResources();

	//RENDER
	Stage::Projection m_ProjStage;
	void CopyToBackBuffer();
	
	std::vector<QuadricMesh*> m_ToRender;

public:
	QuadricRenderer(DX12* pDX12, Camera* pCamera);
	void SetCamera(Camera* pCamera) { m_pCamera = pCamera; }

	void Render();
	void Render(QuadricMesh* pMesh);
};