#include "QuadricRenderer.h"
#include "Helpers.h"
#include "d3dx12.h"
#include <cmath>
#include <d3dcompiler.h>
#include <iostream>
#include "QuadricGeometry.h"
#include <array>

#ifndef USE_PIX
#define USE_PIX
#endif
#include <pix3.h>

using namespace Microsoft::WRL;
using namespace DirectX;

void QuadricRenderer::InitResources(ID3D12GraphicsCommandList* pComList)
{
	GPUResource::BufferParams params;
	params.heapType = D3D12_HEAP_TYPE_DEFAULT;
	params.size = sizeof(AppData);
	m_AppDataBuffer = GPUResource{ m_pDevice, params };
	m_AppDataBuffer.Get()->SetName(L"AppDataBuffer");	
	
	//Output Texture
	GPUResource::Texture2DParams tParams;
	tParams.allowUAV = true;
	tParams.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tParams.width = m_WindowDimensions.width;
	tParams.height = m_WindowDimensions.height;
	tParams.numMips = 1;
	m_OutputBuffer = GPUResource{ m_pDevice, tParams };
	m_OutputBuffer.Get()->SetName(L"OutputTexture");

	//depth Texture
	tParams.format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	m_DepthBuffer = GPUResource{ m_pDevice, tParams };
	m_DepthBuffer.Get()->SetName(L"DepthBuffer");

	//descriptors
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	//DESCRIPTOR HEAP
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_DescriptorHeapSV)));
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

	UINT incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandleSV(m_DescriptorHeapSV->GetCPUDescriptorHandleForHeapStart());
	m_DescriptorHeap->SetName(L"m_DescriptorHeap");
	m_DescriptorHeapSV->SetName(L"m_DescriptorHeapSV");
	m_pDevice->CreateUnorderedAccessView(m_OutputBuffer.Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDevice->CreateUnorderedAccessView(m_OutputBuffer.Get(), nullptr, &uavDesc, srvHeapHandleSV);
	srvHeapHandle.Offset(incrementSize * DescriptorHeapLayout::Depth);
	srvHeapHandleSV.Offset(incrementSize * DescriptorHeapLayout::Depth);
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	m_pDevice->CreateUnorderedAccessView(m_DepthBuffer.Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDevice->CreateUnorderedAccessView(m_DepthBuffer.Get(), nullptr, &uavDesc, srvHeapHandleSV);

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

	std::array<CD3DX12_RESOURCE_BARRIER, 2> transitions{};
	transitions[0] = m_OutputBuffer.TransitionResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	transitions[1] = m_DepthBuffer.TransitionResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pComList->ResourceBarrier((UINT)transitions.size(), transitions.data());


	SetRendererSettings(pComList, 512, {64,64}, 128);
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

void QuadricRenderer::InitDrawCall(ID3D12GraphicsCommandList* pComList)
{
	PIXScopedEvent(pComList, 0, "QuadricRenderer::InitDrawCall");

	CD3DX12_RESOURCE_BARRIER transitions[2]{
		m_RasterizerBuffer.TransitionResource(D3D12_RESOURCE_STATE_COPY_DEST),
		m_ScreenTileBuffer.TransitionResource(D3D12_RESOURCE_STATE_COPY_DEST)
	};
	pComList->ResourceBarrier(2, transitions);

	pComList->CopyResource(m_RasterizerBuffer.Get(), m_RasterizerResetBuffer.Get());
	pComList->CopyResource(m_ScreenTileBuffer.Get(), m_ScreenTileResetBuffer.Get());

	transitions[0] = m_RasterizerBuffer.TransitionResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	transitions[1] = m_ScreenTileBuffer.TransitionResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	pComList->ResourceBarrier(2, transitions);

	//clear gBuffers
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
	UINT incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	UINT max = UINT_MAX;
	pComList->ClearUnorderedAccessViewUint(gpuHandle.Offset(incrementSize * DescriptorHeapLayout::RIndex), cpuHandle.Offset(incrementSize * DescriptorHeapLayout::RIndex)
		, m_RasterizerIBuffer.Get(), &max, 0, nullptr);
	cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
	FLOAT clearVal = (FLOAT)(!(bool)m_AppData.reverseDepth);

	pComList->ClearUnorderedAccessViewFloat(gpuHandle.Offset(incrementSize * DescriptorHeapLayout::RDepth), cpuHandle.Offset(incrementSize * DescriptorHeapLayout::RDepth), m_RasterizerDepthBuffer.Get(), &clearVal, 0, nullptr);

}

void QuadricRenderer::InitRendering(ID3D12GraphicsCommandList* pComList)
{
	PIXScopedEvent(pComList, 0, "QuadricRenderer::InitRendering");

	//update input data : In separate update function ?
	XMStoreFloat4(&m_AppData.lightDirection, XMVector4Normalize(XMVector4Transform(XMVectorSet(0.577f, -0.577f, 0.577f, 0), m_CameraValues.v)));
	m_AppData.viewProjInv = m_CameraValues.vpInv;
	m_AppData.viewInv = m_CameraValues.vInv;
	m_AppData.projInv = XMMatrixInverse(nullptr, m_CameraValues.p);

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
	pComList->SetComputeRootUnorderedAccessView(4, m_RasterizerBuffer.Get()->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(5, m_ScreenTileBuffer.Get()->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(6, m_RasterizerQBuffer.Get()->GetGPUVirtualAddress());
	pComList->SetComputeRootDescriptorTable(7, m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
}

Dimensions<UINT> QuadricRenderer::GetNrTiles() const
{
	auto tDim = m_AppData.tileDimensions;
	return Dimensions<UINT>{(m_WindowDimensions.width /tDim.width + (m_WindowDimensions.width % tDim.width > 0))
		, (m_WindowDimensions.height / tDim.height + (m_WindowDimensions.height % tDim.height > 0)) };
}

QuadricRenderer::QuadricRenderer(ID3D12Device2* pDevice, UINT windowWidth, UINT windowHeight, UINT numBackBuffers)
	:m_pDevice{ pDevice }, m_AppData{}, m_WindowDimensions{ windowWidth ,windowHeight }
	, m_GPStage{}
	,m_RStage{}
	,m_MStage{}
	, m_CameraValues{}
{
	DeferredDeleteQueue::Instance()->SetHysteresis(numBackBuffers * 2);

	m_AppData.windowSize = { windowWidth ,windowHeight, 0, 0 };
	m_AppData.showTiles = false;
	m_AppData.reverseDepth = true;
	m_AppData.tileDimensions = { 128,128 };
	m_AppData.quadricsPerRasterizer = 64;

	SetViewMatrix(DirectX::XMMatrixIdentity());
	SetProjectionVariables(DirectX::XM_PIDIV2, float(windowWidth)/ windowHeight , 1.0f, 100.0f);

	auto tileDim = GetNrTiles();
	UINT screenTiles = tileDim.height * tileDim.width;
	UINT extraRasterizers = screenTiles * 2;
	m_AppData.numRasterizers = screenTiles + extraRasterizers;
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

void QuadricRenderer::SetRendererSettings(ID3D12GraphicsCommandList* pComList, UINT numRasterizers, Dimensions<unsigned int> rasterizerDimensions, UINT quadricsPerRasterizer, bool overrule)
{
	PIXScopedEvent(pComList, 0, "QuadricRenderer::SetRendererSettings");

	if (numRasterizers == 0 || rasterizerDimensions.width == 0 || rasterizerDimensions.height == 0 || quadricsPerRasterizer == 0) return;
	if (numRasterizers == m_AppData.numRasterizers && m_AppData.tileDimensions == rasterizerDimensions && quadricsPerRasterizer == m_AppData.quadricsPerRasterizer) return;

	if (numRasterizers != m_AppData.numRasterizers || m_AppData.tileDimensions != rasterizerDimensions || overrule)
	{
		m_AppData.numRasterizers = numRasterizers;
		m_AppData.tileDimensions = rasterizerDimensions;
		m_AppData.quadricsPerRasterizer = quadricsPerRasterizer;

		//rasterizer buffers
		UINT sqrtNumR = UINT(ceilf(sqrtf((float)m_AppData.numRasterizers)));

		GPUResource::Texture2DParams tParams;
		tParams.allowUAV = true;
		tParams.width = sqrtNumR * (UINT)m_AppData.tileDimensions.width;
		tParams.height = sqrtNumR * (UINT)m_AppData.tileDimensions.height;
		tParams.format = DXGI_FORMAT_R32_UINT;
		tParams.numMips = 1;
		m_RasterizerIBuffer = GPUResource{ m_pDevice, tParams };
		m_RasterizerIBuffer.Get()->SetName(L"RasterizerIndexBuffer");

		tParams.format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		m_RasterizerDepthBuffer = GPUResource{ m_pDevice, tParams };
		m_RasterizerDepthBuffer.Get()->SetName(L"RasterizerDepthBuffer");

		//descriptors
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		UINT incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE shaderVisibleHeapHandle(m_DescriptorHeapSV->GetCPUDescriptorHandleForHeapStart());
		heapHandle.Offset(incrementSize * DescriptorHeapLayout::RIndex);
		shaderVisibleHeapHandle.Offset(incrementSize * DescriptorHeapLayout::RIndex);
		uavDesc.Format = DXGI_FORMAT_R32_UINT;
		m_pDevice->CreateUnorderedAccessView(m_RasterizerIBuffer.Get(), nullptr, &uavDesc, heapHandle);
		m_pDevice->CreateUnorderedAccessView(m_RasterizerIBuffer.Get(), nullptr, &uavDesc, shaderVisibleHeapHandle);
		heapHandle = (m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		shaderVisibleHeapHandle = (m_DescriptorHeapSV->GetCPUDescriptorHandleForHeapStart());
		heapHandle.Offset(incrementSize * DescriptorHeapLayout::RDepth);
		shaderVisibleHeapHandle.Offset(incrementSize * DescriptorHeapLayout::RDepth);
		uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
		m_pDevice->CreateUnorderedAccessView(m_RasterizerDepthBuffer.Get(), nullptr, &uavDesc, heapHandle);
		m_pDevice->CreateUnorderedAccessView(m_RasterizerDepthBuffer.Get(), nullptr, &uavDesc, shaderVisibleHeapHandle);

		//BUFFERS
		//SCREENTILES
		GPUResource::BufferParams params;
		params.heapType = D3D12_HEAP_TYPE_DEFAULT;
		params.size = (sizeof(ScreenTile) * GetNrTiles().width * GetNrTiles().height);
		params.allowUAV = true;

		m_ScreenTileBuffer = GPUResource{ m_pDevice, params };
		m_ScreenTileBuffer.Get()->SetName(L"ScreenTileBuffer");
		
		params.allowUAV = false;
		m_ScreenTileResetBuffer = GPUResource{ m_pDevice, params };
		m_ScreenTileResetBuffer.Get()->SetName(L"ScreenTileBuffer");

		ScreenTile* tiles = static_cast<ScreenTile*>(m_ScreenTileResetBuffer.Map());
		for (UINT i = 0; i < GetNrTiles().width * GetNrTiles().height; i++)
		{
			tiles[i].rasterizerHint = UINT_MAX;
		}

		m_ScreenTileResetBuffer.Unmap(pComList);

		//RASTERIZERS
		// Create the buffer that will be a UAV with rasterizers
		params.size = (UINT)(sizeof(Rasterizer) * m_AppData.numRasterizers);
		params.allowUAV = true;

		m_RasterizerBuffer = GPUResource{ m_pDevice, params };
		m_RasterizerBuffer.Get()->SetName(L"RasterizerBuffer");

		params.allowUAV = false;
		m_RasterizerResetBuffer = GPUResource{ m_pDevice, params };
		m_RasterizerResetBuffer.Get()->SetName(L"RasterizerResetBuffer");

		Rasterizer initial{ UINT_MAX, UINT_MAX , 0 };

		Rasterizer* rasterizers = static_cast<Rasterizer*>(m_RasterizerResetBuffer.Map());
		for (unsigned int i = 0; i < m_AppData.numRasterizers; i++)
		{
			rasterizers[i] = initial;
		}
		m_RasterizerResetBuffer.Unmap(pComList);

		//QBuffer
		params.size = sizeof(OutQuadric) * m_AppData.numRasterizers * m_AppData.quadricsPerRasterizer;
		params.allowUAV = true;
		m_RasterizerQBuffer = GPUResource{ m_pDevice, params };
		m_RasterizerQBuffer.Get()->SetName(L"RasterizerQBuffer");

		std::array< CD3DX12_RESOURCE_BARRIER, 4> transitions{};
		transitions[0] = m_RasterizerIBuffer.TransitionResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		transitions[1] = m_RasterizerDepthBuffer.TransitionResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		transitions[2] = m_RasterizerResetBuffer.TransitionResource(D3D12_RESOURCE_STATE_COPY_SOURCE);
		transitions[3] = m_ScreenTileResetBuffer.TransitionResource(D3D12_RESOURCE_STATE_COPY_SOURCE);

		pComList->ResourceBarrier((UINT)transitions.size(), transitions.data());
	}
	else if (quadricsPerRasterizer != m_AppData.quadricsPerRasterizer)
	{
		m_AppData.quadricsPerRasterizer = quadricsPerRasterizer;

		//QBuffer
		GPUResource::BufferParams params;
		params.heapType = D3D12_HEAP_TYPE_DEFAULT;
		params.size = sizeof(OutQuadric) * m_AppData.numRasterizers * m_AppData.quadricsPerRasterizer;
		params.allowUAV = true;		

		m_RasterizerQBuffer = GPUResource{m_pDevice, params};
		m_RasterizerQBuffer.Get()->SetName(L"RasterizerQBuffer");
	}
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
	SetRendererSettings(pComList, m_AppData.numRasterizers, m_AppData.tileDimensions, m_AppData.quadricsPerRasterizer,true);

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

	for (QuadricGeometry* pGeo : m_ToRender)
	{
		PIXScopedEvent(pComList, 0, "QuadricRenderer::DrawCall: %s", pGeo->GetName().c_str());
		InitDrawCall(pComList);
		if (m_GPStage.Execute(this, pComList, pGeo))
		{
			m_RStage.Execute(this, pComList);
			m_MStage.Execute(this, pComList);
		}
	}
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
