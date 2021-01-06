#pragma once
#include <vector>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#include <imgui.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include "Structs.h"
#include "Window.h"
#include "RasterizationStage.h"
#include "GeometryProcessingStage.h"
#include "MergeStage.h"
#include <set>

class QuadricGeometry;
class Instance;
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
	enum DescriptorHeapLayout : unsigned int
	{
		Color = 0, RIndex, Depth, RDepth, NumDescriptors
	};
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeapSV;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_OutputBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;

	//RASTERIZERS:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_RasterizerBuffer; //uav buffer, flexible(resize when not big enough?)
	Microsoft::WRL::ComPtr<ID3D12Resource> m_RasterizerResetBuffer; //upload buffer to reset screenTiles
	Microsoft::WRL::ComPtr<ID3D12Resource> m_RasterizerQBuffer; //uav buffer with outputQuadrics
	Microsoft::WRL::ComPtr<ID3D12Resource> m_RasterizerDepthBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_RasterizerIBuffer;
	
	//SCREENTILES
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScreenTileBuffer; //uav buffer, flexible(resize when not big enough?)
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScreenTileResetBuffer; //upload buffer to reset screenTiles

	//RS
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

	//Initialization
	void InitResources();
	void InitDrawCall();
	void InitRendering();
	//RENDER STAGES

	friend class Stage::GeometryProcessing;
	friend class Stage::Rasterization;
	friend class Stage::Merge;

	Stage::GeometryProcessing m_GPStage;
	Stage::Rasterization m_RStage;
	Stage::Merge m_MStage;


	void CopyToBackBuffer();
	
	std::set<QuadricGeometry*> m_ToRender;

	DirectX::XMFLOAT4 m_ClearColor = { 66 / 255.0f,135 / 255.0f,245 / 255.0f,0 };
	float m_DepthClearValue = FLT_MAX;

	Dimensions<UINT> GetNrTiles() const;

public:
	QuadricRenderer(DX12* pDX12, Camera* pCamera);
	void SetCamera(Camera* pCamera) { m_pCamera = pCamera; }

	void Render();
	void Render(Instance& instance);
	void Render(QuadricGeometry* pGeo);
	void Render(QuadricGeometry* pGeo, Transform& transform);
	void Render(QuadricGeometry* pGeo, const DirectX::XMMATRIX& transform);
};