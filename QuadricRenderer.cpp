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

	//depth Texture
	texDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;

	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_DepthTexture)));

	//tile G buffers

	texDesc.Width = (UINT)(m_pDX12->GetWindow()->GetDimensions().width * m_AppData.multiplier);
	texDesc.Height = (UINT)(m_pDX12->GetWindow()->GetDimensions().height * m_AppData.multiplier);
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_TileGBuffers[GBUFFER::Color])));

	texDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_TileGBuffers[GBUFFER::Depth])));

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
	ThrowIfFailed(m_pDX12->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_DescriptorHeapShaderVisible)));
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_pDX12->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

	UINT incrementSize = m_pDX12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapShaderVisibleHandle(m_DescriptorHeapShaderVisible->GetCPUDescriptorHandleForHeapStart());
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_OutputTexture.Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_OutputTexture.Get(), nullptr, &uavDesc, srvHeapShaderVisibleHandle);
	srvHeapHandle.Offset(incrementSize);
	srvHeapShaderVisibleHandle.Offset(incrementSize);

	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_DepthTexture.Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_DepthTexture.Get(), nullptr, &uavDesc, srvHeapShaderVisibleHandle);

	//BUFFERS
	unsigned int horizontalTiles{( m_pDX12->GetWindow()->GetDimensions().width / m_AppData.tileDimensions.width) + 1}
	, verticalTiles{ (m_pDX12->GetWindow()->GetDimensions().height / m_AppData.tileDimensions.height) + 1 }
	, nrTiles{ horizontalTiles * verticalTiles };
	// Create the buffer that will be a UAV with screenTiles
	auto byteSize = sizeof(ScreenTile) * nrTiles;
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
		IID_PPV_ARGS(&m_ScreenTileUploadBuffer)));

	ScreenTile initialScreenTile{ UINT_MAX, 0 };
	ScreenTile* screenTiles = new ScreenTile[nrTiles];
	for (size_t i = 0; i < nrTiles; i++)
	{
		screenTiles[i] = initialScreenTile;
	}
	ScreenTile* mapped = nullptr;
	m_ScreenTileUploadBuffer->Map(0, nullptr,
		reinterpret_cast<void**>(&mapped));
	memcpy(mapped, screenTiles, sizeof(ScreenTile) * nrTiles);
	if (m_ScreenTileUploadBuffer != nullptr)
		m_ScreenTileUploadBuffer->Unmap(0, nullptr);

	delete[] screenTiles;

	// Create the buffer that will be a UAV with tiles : initial size is the same as screentiles
	byteSize = (UINT)(sizeof(Tile) * nrTiles * m_AppData.multiplier);
	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&m_TileBuffer)));

	//quadricDistribution
	byteSize = sizeof(unsigned int) * nrTiles * m_QuadricsPerTile;
	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&m_QuadricDistributionBuffer)));

	//init resources
	m_ScreenTileBuffer->SetName(L"m_ScreenTileBuffer");
	m_TileBuffer->SetName(L"m_TileBuffer");
	m_ScreenTileUploadBuffer->SetName(L"m_ScreenTileUploadBuffer");
	m_QuadricDistributionBuffer->SetName(L"m_QuadricDistributionBuffer");


	auto pPipeline = m_pDX12->GetPipeline();
	ThrowIfFailed(pPipeline->commandAllocator->Reset());
	ThrowIfFailed(pPipeline->commandList->Reset(pPipeline->commandAllocator.Get(), nullptr));

	std::vector< CD3DX12_RESOURCE_BARRIER>transitions{};
	transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_OutputTexture.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_DepthTexture.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

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
	CD3DX12_RESOURCE_BARRIER transition{
		CD3DX12_RESOURCE_BARRIER::Transition(m_ScreenTileBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST),
	};
	pComList->ResourceBarrier(1, &transition);

	pPipeline->commandList->CopyResource(m_ScreenTileBuffer.Get(), m_ScreenTileUploadBuffer.Get());

	transition = CD3DX12_RESOURCE_BARRIER::Transition(m_ScreenTileBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST
		, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pComList->ResourceBarrier(1, &transition);
}

QuadricRenderer::QuadricRenderer(DX12* pDX12, Camera* pCamera)
	:m_pDX12{ pDX12 }, m_pCamera{ pCamera }, m_AppData{}
	, m_ProjStage{pDX12}
	, m_TileSelectionStage{pDX12}
	, m_BinningStage{pDX12}
	, m_RasterizationStage{pDX12}
{
	m_AppData.windowSize = { m_pDX12->GetWindow()->GetDimensions().width ,m_pDX12->GetWindow()->GetDimensions().height, 0, 0 };
	m_AppData.tileDimensions = {64,64};
	m_AppData.quadricsPerTile = m_QuadricsPerTile;
	m_AppData.multiplier = 1.0f;
	InitResources();
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
	auto gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapShaderVisible->GetGPUDescriptorHandleForHeapStart());
	UINT incrementSize = m_pDX12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	FLOAT col[4]{ 66 / 255.0f,135 / 255.0f,245 / 255.0f,0 };
	m_pDX12->GetPipeline()->commandList->ClearUnorderedAccessViewFloat(gpuHandle, cpuHandle, m_OutputTexture.Get(), col, 0, nullptr);
	col[0] = FLT_MAX; //max DEPTH
	m_pDX12->GetPipeline()->commandList->ClearUnorderedAccessViewFloat(gpuHandle.Offset(incrementSize), cpuHandle.Offset(incrementSize), m_DepthTexture.Get(), col, 0, nullptr);
	
	
	for (QuadricMesh* pMesh : m_ToRender)
	{
		InitDrawCall();
		m_ProjStage.Execute(this, pMesh);
		m_TileSelectionStage.Execute(this);
		m_BinningStage.Execute(this, pMesh);
		m_RasterizationStage.Execute(this, pMesh);
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
