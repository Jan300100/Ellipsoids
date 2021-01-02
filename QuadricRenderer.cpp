#include "QuadricRenderer.h"
#include "DX12.h"
#include "Helpers.h"
#include "d3dx12.h"
#include <cmath>
#include "Window.h"
#include <d3dcompiler.h>
#include <iostream>
#include "Camera.h"
#include "QuadricMesh.h"

using namespace DirectX;

void QuadricRenderer::InitResources()
{
	//Constant Buffer
	CD3DX12_HEAP_PROPERTIES properties = { CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
	CD3DX12_RESOURCE_DESC desc = { CD3DX12_RESOURCE_DESC::Buffer((sizeof(AppData) + 255) & ~255) };
	m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_AppDataBuffer));

	//Output Texture
	D3D12_RESOURCE_DESC texDesc;

	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_pDX12->GetWindow()->GetDimensions().width;
	texDesc.Height = m_pDX12->GetWindow()->GetDimensions().height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_OutputTexture)));
	m_OutputTexture->SetName(L"m_OutputTexture");

	//depth Texture
	texDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;

	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_DepthTexture)));
	m_DepthTexture->SetName(L"m_DepthTexture");


	//tile G buffers

	texDesc.Width = (UINT(ceilf(sqrtf((float)m_AppData.numRasterizers))) * m_AppData.tileDimensions.width);
	texDesc.Height = (UINT(ceilf(sqrtf((float)m_AppData.numRasterizers))) * m_AppData.tileDimensions.height);
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_RasterizerGBuffers[GBUFFER::Color])));
	m_RasterizerGBuffers[GBUFFER::Color]->SetName(L"GBufferColor");
	texDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_RasterizerGBuffers[GBUFFER::Depth])));
	m_RasterizerGBuffers[GBUFFER::Depth]->SetName(L"GBufferDepth");

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
	ThrowIfFailed(m_pDX12->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_DescriptorHeapSV)));
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_pDX12->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

	UINT incrementSize = m_pDX12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandleSV(m_DescriptorHeapSV->GetCPUDescriptorHandleForHeapStart());
	m_DescriptorHeapSV->SetName(L"m_DescriptorHeapSV");
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_OutputTexture.Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_OutputTexture.Get(), nullptr, &uavDesc, srvHeapHandleSV);
	srvHeapHandle.Offset(incrementSize);
	srvHeapHandleSV.Offset(incrementSize);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_RasterizerGBuffers[GBUFFER::Color].Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_RasterizerGBuffers[GBUFFER::Color].Get(), nullptr, &uavDesc, srvHeapHandleSV);
	srvHeapHandle.Offset(incrementSize);
	srvHeapHandleSV.Offset(incrementSize);
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_DepthTexture.Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_DepthTexture.Get(), nullptr, &uavDesc, srvHeapHandleSV);
	srvHeapHandle.Offset(incrementSize);
	srvHeapHandleSV.Offset(incrementSize);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_RasterizerGBuffers[GBUFFER::Depth].Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_RasterizerGBuffers[GBUFFER::Depth].Get(), nullptr, &uavDesc, srvHeapHandleSV);

	
	//BUFFERS
	//SCREENTILES

	UINT byteSize = (sizeof(ScreenTile) * GetNrTiles().width * GetNrTiles().height);
	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&m_ScreenTileBuffer)));

	//uploadBuffer
	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_ScreenTileResetBuffer)));

	ScreenTile* tiles = nullptr;
	m_ScreenTileResetBuffer->Map(0, nullptr,
		reinterpret_cast<void**>(&tiles));
	for (UINT i = 0; i < GetNrTiles().width * GetNrTiles().height; i++)
	{
		tiles[i].rasterizerHint = UINT_MAX;
	}

	if (m_ScreenTileResetBuffer != nullptr)
		m_ScreenTileResetBuffer->Unmap(0, nullptr);

	//RASTERIZERS
	// Create the buffer that will be a UAV with rasterizers
	byteSize = (UINT)(sizeof(Rasterizer) * m_AppData.numRasterizers);
	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&m_RasterizerBuffer)));

	//uploadBuffer
	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_RasterizerResetBuffer)));

	Rasterizer initial{ UINT_MAX, 0, UINT_MAX , 0};

	Rasterizer* rasterizers = nullptr;
	m_RasterizerResetBuffer->Map(0, nullptr,
		reinterpret_cast<void**>(&rasterizers));
	for (unsigned int i = 0; i < m_AppData.numRasterizers; i++)
	{
		initial.rasterizerIdx = i;
		rasterizers[i] = initial;
	}
	if (m_RasterizerResetBuffer != nullptr)
		m_RasterizerResetBuffer->Unmap(0, nullptr);


	//QBuffer
	byteSize = sizeof(OutQuadric) * m_AppData.numRasterizers * m_AppData.quadricsPerRasterizer;
	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&m_RasterizerQBuffer)));

	//init resources
	m_RasterizerBuffer->SetName(L"m_RasterizerBuffer");
	m_RasterizerResetBuffer->SetName(L"m_RasterizerResetBuffer");
	m_RasterizerQBuffer->SetName(L"m_RasterizerQBuffer");


	auto pPipeline = m_pDX12->GetPipeline();
	ThrowIfFailed(pPipeline->commandAllocator->Reset());
	ThrowIfFailed(pPipeline->commandList->Reset(pPipeline->commandAllocator.Get(), nullptr));

	std::vector< CD3DX12_RESOURCE_BARRIER>transitions{};
	transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_OutputTexture.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_DepthTexture.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	
	for (UINT i = 0; i < GBUFFER::NumBuffers; i++)
	{
		transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_RasterizerGBuffers[i].Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}
	
	pPipeline->commandList->ResourceBarrier((UINT)transitions.size(), transitions.data());

	ThrowIfFailed(pPipeline->commandList->Close());
	ID3D12CommandList* cmdsLists[] = { pPipeline->commandList.Get() };
	pPipeline->commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	pPipeline->Flush();
}

void QuadricRenderer::CopyToBackBuffer()
{
	auto pPipeline = m_pDX12->GetPipeline();
	auto pComList = pPipeline->commandList;
	//COPY COMPUTE BUFFER to Current Backbuffer to present it
	CD3DX12_RESOURCE_BARRIER transitions[2]
	{
		CD3DX12_RESOURCE_BARRIER::Transition(pPipeline->GetCurrentRenderTarget(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST),
		CD3DX12_RESOURCE_BARRIER::Transition(m_OutputTexture.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE)
	};
	pComList->ResourceBarrier(2, transitions);

	pComList->CopyResource(pPipeline->GetCurrentRenderTarget(), m_OutputTexture.Get());

	// Transition to Render target for any other draws that might happen.
	transitions[0] = CD3DX12_RESOURCE_BARRIER::Transition(pPipeline->GetCurrentRenderTarget(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	transitions[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_OutputTexture.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pComList->ResourceBarrier(2, transitions);
}

void QuadricRenderer::InitDrawCall()
{
	auto pPipeline = m_pDX12->GetPipeline();
	auto pComList = pPipeline->commandList;
	CD3DX12_RESOURCE_BARRIER transitions[2]{
		CD3DX12_RESOURCE_BARRIER::Transition(m_RasterizerBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST),
		CD3DX12_RESOURCE_BARRIER::Transition(m_ScreenTileBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST)
	};
	pComList->ResourceBarrier(2, transitions);

	pPipeline->commandList->CopyResource(m_RasterizerBuffer.Get(), m_RasterizerResetBuffer.Get());
	pPipeline->commandList->CopyResource(m_ScreenTileBuffer.Get(), m_ScreenTileResetBuffer.Get());

	transitions[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_RasterizerBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST
		, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	transitions[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_ScreenTileBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST
		, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pComList->ResourceBarrier(2, transitions);


	//clear gBuffers
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
	UINT incrementSize = m_pDX12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDX12->GetPipeline()->commandList->ClearUnorderedAccessViewFloat(gpuHandle.Offset(incrementSize * DescriptorHeapLayout::GColor), cpuHandle.Offset(incrementSize * DescriptorHeapLayout::GColor), m_RasterizerGBuffers[GBUFFER::Color].Get(), (FLOAT*)&m_ClearColor, 0, nullptr);
	cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
	m_pDX12->GetPipeline()->commandList->ClearUnorderedAccessViewFloat(gpuHandle.Offset(incrementSize * DescriptorHeapLayout::GDepth), cpuHandle.Offset(incrementSize * DescriptorHeapLayout::GDepth), m_RasterizerGBuffers[GBUFFER::Depth].Get(), (FLOAT*)&m_DepthClearValue, 0, nullptr);
}

void QuadricRenderer::PrepareMeshes()
{
	for (QuadricMesh* pMesh : m_ToRender)
	{
		pMesh->ReadMeshOutput();
	}

	auto pPipeline = m_pDX12->GetPipeline();
	auto pComList = pPipeline->commandList;
	std::vector< CD3DX12_RESOURCE_BARRIER> barriers{};
	for (QuadricMesh* pMesh : m_ToRender)
	{
		barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(pMesh->GetMeshOutputBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			, D3D12_RESOURCE_STATE_COPY_SOURCE));
	}
	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());
	barriers.clear();
	for (QuadricMesh* pMesh : m_ToRender)
	{
		pComList->CopyResource(pMesh->GetMeshOutputReadbackBuffer(), pMesh->GetMeshOutputBuffer());

		barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(pMesh->GetMeshOutputBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE
			, D3D12_RESOURCE_STATE_COPY_DEST));
	}
	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());
	barriers.clear();
	for (QuadricMesh* pMesh : m_ToRender)
	{
		pComList->CopyResource(pMesh->GetMeshOutputBuffer(), pMesh->GetMeshOutputResetBuffer());

		barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(pMesh->GetMeshOutputBuffer(), D3D12_RESOURCE_STATE_COPY_DEST
			, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}
	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());
}

Dimensions<UINT> QuadricRenderer::GetNrTiles() const
{
	auto wDim = m_pDX12->GetWindow()->GetDimensions();
	auto tDim = m_AppData.tileDimensions;
	return Dimensions<UINT>{(wDim.width /tDim.width + (wDim.width % tDim.width > 0)), (wDim.height / tDim.height + (wDim.height % tDim.height > 0)) };
}

QuadricRenderer::QuadricRenderer(DX12* pDX12, Camera* pCamera)
	:m_pDX12{ pDX12 }, m_pCamera{ pCamera }, m_AppData{}
	, m_GPStage{pDX12}
	,m_RStage{pDX12}
	,m_MStage{pDX12}
{
	m_AppData.windowSize = { m_pDX12->GetWindow()->GetDimensions().width ,m_pDX12->GetWindow()->GetDimensions().height, 0, 0 };
	m_AppData.tileDimensions = { 256,32 };
	m_AppData.quadricsPerRasterizer = 128;

	UINT screenTiles = GetNrTiles().height * GetNrTiles().width;
	UINT extraRasterizers = screenTiles;
	m_AppData.numRasterizers = screenTiles + extraRasterizers;

	InitResources();

	m_GPStage.Init(this);
	m_RStage.Init(this);
	m_MStage.Init(this);
}

void QuadricRenderer::Render()
{
	//update input data : In separate update function ?
	XMStoreFloat4(&m_AppData.lightDirection, XMVector4Normalize(XMVector4Transform(XMVectorSet(0.577f, -0.577f, 0.577f, 0), m_pCamera->GetView())));
	m_AppData.viewProjInv = m_pCamera->GetViewProjectionInverse();
	m_AppData.viewInv = m_pCamera->GetViewInverse();
	m_AppData.projInv = XMMatrixInverse(nullptr, m_pCamera->GetViewProjection());

	BYTE* mapped = nullptr;
	m_AppDataBuffer->Map(0, nullptr,
		reinterpret_cast<void**>(&mapped));
	memcpy(mapped, &m_AppData, sizeof(AppData));
	if (m_AppDataBuffer != nullptr)
		m_AppDataBuffer->Unmap(0, nullptr);

	// Clear the buffers
	// https://www.gamedev.net/forums/topic/672063-d3d12-clearunorderedaccessviewfloat-fails/
	auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	auto gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
	UINT incrementSize = m_pDX12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_pDX12->GetPipeline()->commandList->ClearUnorderedAccessViewFloat(
		gpuHandle.Offset(incrementSize * DescriptorHeapLayout::Color),
		cpuHandle.Offset(incrementSize * DescriptorHeapLayout::Color),
		m_OutputTexture.Get(), (FLOAT*)&m_ClearColor, 0, nullptr);
	cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
	m_pDX12->GetPipeline()->commandList->ClearUnorderedAccessViewFloat(
		gpuHandle.Offset(incrementSize * DescriptorHeapLayout::Depth), 
		cpuHandle.Offset(incrementSize * DescriptorHeapLayout::Depth), 
		m_DepthTexture.Get(), (FLOAT*)&m_DepthClearValue, 0, nullptr);
	
	PrepareMeshes();

	for (size_t rendered = 0; rendered < m_ToRender.size();)
	{
		InitDrawCall();
		rendered = m_GPStage.Execute(this, m_ToRender, (UINT)rendered);
		m_RStage.Execute(this);
		m_MStage.Execute(this);
	}

	CopyToBackBuffer();

	//reset for next frame
	m_ToRender.clear();
}

void QuadricRenderer::Render(QuadricMesh* pMesh)
{
	//registers the quadric for rendering
	m_ToRender.push_back(pMesh);
}
