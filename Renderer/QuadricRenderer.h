#pragma once
#include <vector>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include "Structs.h"
#include "RasterizationStage.h"
#include "GeometryProcessingStage.h"
#include "MergeStage.h"
#include <set>
#include <queue>
#include <memory>
#include "DeferredDeleteQueue.h"

class QuadricGeometry;

struct CameraValues
{
	DirectX::XMMATRIX v, vp, vInv, vpInv, p;
	float fov, aspectRatio, nearPlane, farPlane;
};

class QuadricRenderer
{
	friend class Stage::GeometryProcessing;
	friend class Stage::Rasterization;
	friend class Stage::Merge;
private:
	ID3D12Device2* m_pDevice;

	AppData m_AppData;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_AppDataBuffer; //general data (for both stages?)

	// Shared Root Signature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

	enum DescriptorHeapLayout : unsigned int
	{
		Color = 0, RIndex, Depth, RDepth, NumDescriptors
	};

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeapSV;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_OutputBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;

	struct BatchData
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> m_InputIndices; // points to which inputQuadric to render in this batch. (instanceIdx, instanceBufferIdx, quadricIdx, meshIdx, ...)
		Microsoft::WRL::ComPtr<ID3D12Resource> m_OutputQuadrics;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_OutputBins; // bin of indices per tile
	};

	// STAGES
	Stage::GeometryProcessing m_GPStage;
	Stage::Rasterization m_RStage;
	Stage::Merge m_MStage;

	void InitResources(ID3D12GraphicsCommandList* pComList);
	void InitRendering(ID3D12GraphicsCommandList* pComList);
	void CopyToBackBuffer(ID3D12GraphicsCommandList* pComList, ID3D12Resource* pRenderTarget, ID3D12Resource* pDepthBuffer);

	std::set<QuadricGeometry*> m_ToRender;

	DirectX::XMFLOAT4 m_ClearColor = { 66 / 255.0f,135 / 255.0f,245 / 255.0f,0 };

	Dimensions<UINT> GetNrTiles() const;
	CameraValues m_CameraValues;
	bool m_Initialized = false;
	Dimensions<UINT> m_WindowDimensions;

	std::unique_ptr<DeferredDeleteQueue> m_DeferredDeleteQueue;
public:
	QuadricRenderer(ID3D12Device2* pDevice, UINT windowWidth, UINT windowHeight, UINT numBackBuffers);
	~QuadricRenderer() = default;
	QuadricRenderer(const QuadricRenderer& other) = delete;
	QuadricRenderer(QuadricRenderer&&) = delete;
	QuadricRenderer& operator=(const QuadricRenderer& other) = delete;
	QuadricRenderer& operator=(QuadricRenderer&&) = delete;

	ID3D12Device2* GetDevice() const;
	void SetViewMatrix(const DirectX::XMMATRIX& view);

	void SetClearColor(float r, float g, float b, float a);
	void ShowTiles(bool show);
	void ReverseDepth(bool reverse);
	void SetProjectionVariables(float fov, float aspectRatio, float nearPlane, float farPlane);

	void Initialize(ID3D12GraphicsCommandList* pComList);

	void RenderFrame2(ID3D12GraphicsCommandList* pComList, ID3D12Resource* pRenderTarget, ID3D12Resource* pDepthBuffer = nullptr);

	void RenderFrame(ID3D12GraphicsCommandList* pComList, ID3D12Resource* pRenderTarget, ID3D12Resource* pDepthBuffer = nullptr);
	void Render(QuadricGeometry* pGeo);
	void Render(QuadricGeometry* pGeo, const DirectX::XMMATRIX& transform);

	DeferredDeleteQueue* GetDeferredDeleteQueue() const;
};