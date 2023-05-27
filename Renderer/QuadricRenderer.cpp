#include "QuadricRenderer.h"
#include "Helpers.h"
#include "d3dx12.h"
#include <cmath>
#include <d3dcompiler.h>
#include <iostream>
#include "QuadricGeometry.h"

#ifndef USE_PIX
#define USE_PIX
#endif
#include <pix3.h>

using namespace Microsoft::WRL;
using namespace DirectX;

void QuadricRenderer::CreateBatch()
{
	GPUResource::BufferParams params;

	params.size = sizeof(uint32_t) * 4 * m_AppData.batchSize;
	params.heapType = D3D12_HEAP_TYPE_DEFAULT;
	params.allowUAV = false;
	m_batchBuffers.inputIndices = GPUResource{ m_pDevice, params };
	m_batchBuffers.inputIndices.Get()->SetName(L"batchInputIndices");

	params.size = sizeof(OutQuadric) * m_AppData.batchSize;
	params.allowUAV = true;
	m_batchBuffers.outputQuadrics = GPUResource{ m_pDevice, params };
	m_batchBuffers.outputQuadrics.Get()->SetName(L"batchOutputQuadrics");

	auto nrTiles = GetNrTiles();
	params.size = m_AppData.batchSize * sizeof(uint32_t) * nrTiles.width * nrTiles.height; // can do with way less bits per index instead of sizeof(int) (log2(batchSize))
	m_batchBuffers.outputBins = GPUResource{ m_pDevice, params };
	m_batchBuffers.outputBins.Get()->SetName(L"batchOutputBins");
}

void QuadricRenderer::InitResources(ID3D12GraphicsCommandList* pComList)
{
	GPUResource::BufferParams params;
	params.size = sizeof(AppData);
	params.heapType = D3D12_HEAP_TYPE_DEFAULT;
	params.allowUAV = false;
	m_AppDataBuffer = GPUResource{ m_pDevice, params };
	m_AppDataBuffer.Get()->SetName(L"AppDataBuffer");	
	
	GPUResource::Texture2DParams tParams;
	tParams.allowUAV = true;
	tParams.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tParams.width = m_WindowDimensions.width;
	tParams.height = m_WindowDimensions.height;
	tParams.numMips = 1;

	m_OutputBuffer = GPUResource{ m_pDevice, tParams };
	m_OutputBuffer.Get()->SetName(L"OutputTexture");

	tParams.format = DXGI_FORMAT_R32_FLOAT;
	m_DepthBuffer = GPUResource{ m_pDevice, tParams };
	m_DepthBuffer.Get()->SetName(L"DepthBuffer");

	CreateBatch();

	//descriptors
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	//DESCRIPTOR HEAP
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 4;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_DescriptorHeapSV)));
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

	UINT incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE shaderVisibleHeapHandle(m_DescriptorHeapSV->GetCPUDescriptorHandleForHeapStart());
	m_DescriptorHeap->SetName(L"m_DescriptorHeap");
	m_DescriptorHeapSV->SetName(L"m_DescriptorHeapSV");
	m_pDevice->CreateUnorderedAccessView(m_OutputBuffer.Get(), nullptr, &uavDesc, heapHandle);
	m_pDevice->CreateUnorderedAccessView(m_OutputBuffer.Get(), nullptr, &uavDesc, shaderVisibleHeapHandle);
	heapHandle.Offset(incrementSize * DescriptorHeapLayout::Depth);
	shaderVisibleHeapHandle.Offset(incrementSize * DescriptorHeapLayout::Depth);
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	m_pDevice->CreateUnorderedAccessView(m_DepthBuffer.Get(), nullptr, &uavDesc, heapHandle);
	m_pDevice->CreateUnorderedAccessView(m_DepthBuffer.Get(), nullptr, &uavDesc, shaderVisibleHeapHandle);

	//ROOT SIGNATURE
	CD3DX12_ROOT_PARAMETER rootParameter[8];

	CD3DX12_DESCRIPTOR_RANGE range;
	range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 3, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	rootParameter[0].InitAsConstants(1,0);
	rootParameter[1].InitAsConstantBufferView(1);
	rootParameter[2].InitAsShaderResourceView(0);
	rootParameter[3].InitAsShaderResourceView(1);
	rootParameter[4].InitAsUnorderedAccessView(0);
	rootParameter[5].InitAsUnorderedAccessView(1);
	rootParameter[6].InitAsUnorderedAccessView(2);
	rootParameter[7].InitAsDescriptorTable(1, &range);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(8, rootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(m_pDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_RootSignature.GetAddressOf())));

	std::vector<CD3DX12_RESOURCE_BARRIER> transitions{};
	transitions.push_back(m_OutputBuffer.TransitionResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	transitions.push_back(m_DepthBuffer.TransitionResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	pComList->ResourceBarrier((UINT)transitions.size(), transitions.data());
}

void QuadricRenderer::CopyToBackBuffer(ID3D12GraphicsCommandList* pComList, ID3D12Resource* pRenderTarget, ID3D12Resource*)
{
	PIXScopedEvent(pComList, 0, "QuadricRenderer::CopyToBackBuffer");

	D3D12_RESOURCE_STATES ogState = m_OutputBuffer.GetCurrentState();

	//COPY COMPUTE BUFFER to Current Backbuffer to present it
	CD3DX12_RESOURCE_BARRIER transitions[2]
	{
		CD3DX12_RESOURCE_BARRIER::Transition(pRenderTarget,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST),
		m_OutputBuffer.TransitionResource(D3D12_RESOURCE_STATE_COPY_SOURCE)
	};
	pComList->ResourceBarrier(2, transitions);

	pComList->CopyResource(pRenderTarget, m_OutputBuffer.Get());

	// Transition to Render target for any other draws that might happen.
	transitions[0] = CD3DX12_RESOURCE_BARRIER::Transition(pRenderTarget,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	transitions[1] = m_OutputBuffer.TransitionResource(ogState);
	pComList->ResourceBarrier(2, transitions);
}

void QuadricRenderer::InitRendering(ID3D12GraphicsCommandList* pComList)
{
	PIXScopedEvent(pComList, 0, "QuadricRenderer::InitRendering");

	//update input data : In separate update function ?
	XMStoreFloat4(&m_AppData.lightDirection, XMVector4Normalize(XMVector4Transform(XMVectorSet(0.577f, -0.577f, 0.577f, 0), m_CameraValues.v)));
	m_AppData.viewProjInv = m_CameraValues.vpInv;
	m_AppData.viewInv = m_CameraValues.vInv;
	m_AppData.projInv = XMMatrixInverse(nullptr, m_CameraValues.p);
	m_AppData.batchSize = 32;

	void* mapped = m_AppDataBuffer.Map();
	memcpy(mapped, &m_AppData, sizeof(AppData));
	m_AppDataBuffer.Unmap(pComList);

	// Set Descriptor Heaps
	ID3D12DescriptorHeap* descHeaps[]{ m_DescriptorHeapSV.Get() };
	pComList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	// Clear the buffers
	// https://www.gamedev.net/forums/topic/672063-d3d12-clearunorderedaccessviewfloat-fails/
	auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	auto gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
	UINT incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pComList->ClearUnorderedAccessViewFloat(
		gpuHandle.Offset(incrementSize * DescriptorHeapLayout::Color),
		cpuHandle.Offset(incrementSize * DescriptorHeapLayout::Color),
		m_OutputBuffer.Get(), (FLOAT*)&m_ClearColor, 0, nullptr);
	cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());

	FLOAT clearVal = (FLOAT)(!(bool)m_AppData.reverseDepth);

	pComList->ClearUnorderedAccessViewFloat(
		gpuHandle.Offset(incrementSize * DescriptorHeapLayout::Depth),
		cpuHandle.Offset(incrementSize * DescriptorHeapLayout::Depth),
		
		m_DepthBuffer.Get(), &clearVal, 0, nullptr);

	//set root sign and parameters
	pComList->SetComputeRootSignature(m_RootSignature.Get());

	pComList->SetComputeRootConstantBufferView(1,m_AppDataBuffer.Get()->GetGPUVirtualAddress());
	// set bindless inputquadric buffers and instance buffers
	pComList->SetComputeRootDescriptorTable(7, m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
}

Dimensions<UINT> QuadricRenderer::GetNrTiles() const
{
	auto tDim = m_AppData.tileDimensions;
	return Dimensions<UINT>{(m_WindowDimensions.width /tDim.width + (m_WindowDimensions.width % tDim.width > 0))
		, (m_WindowDimensions.height / tDim.height + (m_WindowDimensions.height % tDim.height > 0)) };
}

QuadricRenderer::QuadricRenderer(ID3D12Device2* pDevice, UINT windowWidth, UINT windowHeight, UINT numBackBuffers)
	: m_pDevice{ pDevice }, m_AppData{}, m_WindowDimensions{ windowWidth ,windowHeight }
	, m_GPStage{}
	, m_RStage{}
	, m_MStage{}
	, m_CameraValues{}
	, m_batchBuffers{}
{
	DeferredDeleteQueue::Instance()->SetHysteresis(numBackBuffers * 2);

	m_AppData.windowSize = { windowWidth ,windowHeight, 0, 0 };
	m_AppData.showTiles = false;
	m_AppData.reverseDepth = true;
	m_AppData.tileDimensions = { 128,128 };
	m_AppData.batchSize = 32;

	SetViewMatrix(DirectX::XMMatrixIdentity());
	SetProjectionVariables(DirectX::XM_PIDIV2, float(windowWidth)/ windowHeight , 1.0f, 100.0f);
}

ID3D12Device2* QuadricRenderer::GetDevice() const
{
	return m_pDevice;
}

void QuadricRenderer::SetViewMatrix(const DirectX::XMMATRIX& view)
{
	m_CameraValues.v = view;
	m_CameraValues.vInv = XMMatrixInverse(nullptr, view);
	m_CameraValues.vp = view * m_CameraValues.p;
	m_CameraValues.vpInv = XMMatrixInverse(nullptr, m_CameraValues.vp);
}

void QuadricRenderer::SetClearColor(float r, float g, float b, float a)
{
	m_ClearColor = { r,g,b,a };
}

void QuadricRenderer::ShowTiles(bool show)
{
	m_AppData.showTiles = show;
}

void QuadricRenderer::ReverseDepth(bool reverse)
{
	m_AppData.reverseDepth = reverse;
	SetProjectionVariables(m_CameraValues.fov, m_CameraValues.aspectRatio, m_CameraValues.nearPlane, m_CameraValues.farPlane);
}

void QuadricRenderer::SetProjectionVariables(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	m_CameraValues.fov = fov;
	m_CameraValues.aspectRatio = aspectRatio;
	m_CameraValues.nearPlane = nearPlane;
	m_CameraValues.farPlane = farPlane;

	if (m_AppData.reverseDepth)
	{
		m_CameraValues.p = XMMatrixPerspectiveFovLH(fov, aspectRatio, farPlane, nearPlane);
	}
	else
	{
		m_CameraValues.p = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
	}
	SetViewMatrix(m_CameraValues.v); //recalc matrices;
}

void QuadricRenderer::Initialize(ID3D12GraphicsCommandList* pComList)
{
	PIXScopedEvent(pComList, 0, "QuadricRenderer::Initialize");

	InitResources(pComList);

	m_GPStage.Init(this);
	m_RStage.Init(this);
	m_MStage.Init(this);

	m_Initialized = true;
}

void QuadricRenderer::RenderFrame(ID3D12GraphicsCommandList* pComList, ID3D12Resource* pRenderTarget, ID3D12Resource* pDepthBuffer)
{
	PIXScopedEvent(pComList,0, "QuadricRenderer::RenderFrame");

	if (!m_Initialized) throw std::wstring{ L"Quadric Renderer not initialized!" };
	
	DeferredDeleteQueue::Instance()->BeginFrame();

	InitRendering(pComList);

	//for i < n: for (QuadricGeometry* pGeo : m_ToRender)
	
		// batch geometry per n (e.g. 64) quadrics
		// fill batches indices :
		// for each quadric: 
		// [x,y,z,w] * n //inputQ 'indices'
			// x = inQ idx; //which quadric in the 'meshBuffer'?
			// y = instance idx; // which instance in instanceBuffer?
			// z = instanceBuffer idx // which instanceBuffer?
			// w = inQBuffer idx // which 'meshBuffer'?	}
	

	// allocate(reuse really) batch memory: n * 64 * sizeof(outquadric) UAV write memory.

	// 2. copy to gpu.

	// dispatch geometryStage for batch 1, make sure all inputQBuffers are on gpu available.

	// allocate(reuse really) n * (screenW / tileW) * (screenH / tileH) ; // tileMemory, each tile has enough memory to store [batchsize] indices.

	// dispatch binning stage

	// dispatch rasterizationstage, which writes to rendertarget. 


	m_ToRender.clear();

	CopyToBackBuffer(pComList, pRenderTarget, pDepthBuffer);
}

void QuadricRenderer::Render(QuadricGeometry* pGeo)
{
	Render(pGeo, XMMatrixIdentity());
}

void QuadricRenderer::Render(QuadricGeometry* pGeo, const DirectX::XMMATRIX& transform)
{
	m_ToRender.insert(pGeo);
	auto tr = XMMatrixInverse(nullptr, XMMatrixTranspose(transform));
	pGeo->m_Transforms.push_back(tr);
}
