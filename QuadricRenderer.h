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
#include "Window.h"
#include "TileSelectionStage.h"
#include "BinningStage.h"
#include "RasterizationStage.h"

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
	enum GBuffer : unsigned int
	{
		Depth = 0, Color, NumBuffers
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_TileGBuffers[GBuffer::NumBuffers];
	//TODO:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScreenTileBuffer; //uav buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> m_TileBuffer; //uav buffer, flexible(resize when not big enough?)
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScreenTileUploadBuffer; //upload buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> m_QuadricDistributionBuffer; //uav buffer
	unsigned int m_QuadricsPerTile = 256;
	Dimensions<unsigned int> m_TileDimensions = {64,64};
	
	//Initialization
	void InitResources();

	//RENDER STAGES
	friend class Stage::Projection;
	friend class Stage::TileSelection;
	friend class Stage::Binning;
	friend class Stage::Rasterization;

	Stage::Projection m_ProjStage;
	Stage::TileSelection m_TileSelectionStage;
	Stage::Binning m_BinningStage;
	Stage::Rasterization m_RasterizationStage;
	void CopyToBackBuffer();
	
	std::vector<QuadricMesh*> m_ToRender;

	void InitDrawCall();

public:
	QuadricRenderer(DX12* pDX12, Camera* pCamera);
	void SetCamera(Camera* pCamera) { m_pCamera = pCamera; }

	void Render();
	void Render(QuadricMesh* pMesh);
};