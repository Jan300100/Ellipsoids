#include "QuadricRenderer.h"
#include "DX12.h"
#include "Helpers.h"
#include "d3dx12.h"
#include <cmath>
#include "Window.h"
#include <d3dcompiler.h>
#include <iostream>
#include "Camera.h"
#include "QuadricGeometry.h"
#include "Instance.h"

using namespace Microsoft::WRL;

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
		IID_PPV_ARGS(&m_OutputBuffer)));
	m_OutputBuffer->SetName(L"m_OutputTexture");

	//depth Texture
	texDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;

	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_DepthBuffer)));
	m_DepthBuffer->SetName(L"DepthBuffer");


	//tile G buffers

	UINT sqrtNumR = UINT(ceilf(sqrtf((float)m_AppData.numRasterizers)));
	texDesc.Width = sqrtNumR * (UINT)m_AppData.tileDimensions.width;
	texDesc.Height = sqrtNumR * (UINT)m_AppData.tileDimensions.height;
	texDesc.Format = DXGI_FORMAT_R32_UINT;
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_RasterizerIBuffer)));
	m_RasterizerIBuffer->SetName(L"RIndexBuffer");

	texDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_RasterizerDepthBuffer)));
	m_RasterizerDepthBuffer->SetName(L"RDepthBuffer");

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
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_OutputBuffer.Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_OutputBuffer.Get(), nullptr, &uavDesc, srvHeapHandleSV);
	srvHeapHandle.Offset(incrementSize);
	srvHeapHandleSV.Offset(incrementSize);
	uavDesc.Format = DXGI_FORMAT_R32_UINT;
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_RasterizerIBuffer.Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_RasterizerIBuffer.Get(), nullptr, &uavDesc, srvHeapHandleSV);
	srvHeapHandle.Offset(incrementSize);
	srvHeapHandleSV.Offset(incrementSize);
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_DepthBuffer.Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_DepthBuffer.Get(), nullptr, &uavDesc, srvHeapHandleSV);
	srvHeapHandle.Offset(incrementSize);
	srvHeapHandleSV.Offset(incrementSize);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_RasterizerDepthBuffer.Get(), nullptr, &uavDesc, srvHeapHandle);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(m_RasterizerDepthBuffer.Get(), nullptr, &uavDesc, srvHeapHandleSV);

	
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

	ThrowIfFailed(m_pDX12->GetDevice()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_RootSignature.GetAddressOf())));



	//init resources
	m_RasterizerBuffer->SetName(L"m_RasterizerBuffer");
	m_RasterizerResetBuffer->SetName(L"m_RasterizerResetBuffer");
	m_RasterizerQBuffer->SetName(L"m_RasterizerQBuffer");


	auto pPipeline = m_pDX12->GetPipeline();
	ThrowIfFailed(pPipeline->commandAllocator->Reset());
	ThrowIfFailed(pPipeline->commandList->Reset(pPipeline->commandAllocator.Get(), nullptr));

	std::vector< CD3DX12_RESOURCE_BARRIER>transitions{};
	transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_OutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_DepthBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	
	transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_RasterizerIBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_RasterizerDepthBuffer.Get(),
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
		CD3DX12_RESOURCE_BARRIER::Transition(m_OutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE)
	};
	pComList->ResourceBarrier(2, transitions);

	pComList->CopyResource(pPipeline->GetCurrentRenderTarget(), m_OutputBuffer.Get());

	// Transition to Render target for any other draws that might happen.
	transitions[0] = CD3DX12_RESOURCE_BARRIER::Transition(pPipeline->GetCurrentRenderTarget(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	transitions[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_OutputBuffer.Get(),
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
	UINT max = UINT_MAX;
	m_pDX12->GetPipeline()->commandList->ClearUnorderedAccessViewUint(gpuHandle.Offset(incrementSize * DescriptorHeapLayout::RIndex), cpuHandle.Offset(incrementSize * DescriptorHeapLayout::RIndex)
		, m_RasterizerIBuffer.Get(), &max, 0, nullptr);
	cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
	m_pDX12->GetPipeline()->commandList->ClearUnorderedAccessViewFloat(gpuHandle.Offset(incrementSize * DescriptorHeapLayout::RDepth), cpuHandle.Offset(incrementSize * DescriptorHeapLayout::RDepth), m_RasterizerDepthBuffer.Get(), (FLOAT*)&m_DepthClearValue, 0, nullptr);

	


}

void QuadricRenderer::InitRendering()
{
	//update input data : In separate update function ?
	XMStoreFloat4(&m_AppData.lightDirection, XMVector4Normalize(XMVector4Transform(XMVectorSet(0.577f, -0.577f, 0.577f, 0), m_pCamera->GetView())));
	m_AppData.viewProjInv = m_pCamera->GetViewProjectionInverse();
	m_AppData.viewInv = m_pCamera->GetViewInverse();
	m_AppData.projInv = XMMatrixInverse(nullptr, m_pCamera->GetViewProjection());

	BYTE* mapped = nullptr;
	ThrowIfFailed(m_AppDataBuffer->Map(0, nullptr,
		reinterpret_cast<void**>(&mapped)));
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
		m_OutputBuffer.Get(), (FLOAT*)&m_ClearColor, 0, nullptr);
	cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
	m_pDX12->GetPipeline()->commandList->ClearUnorderedAccessViewFloat(
		gpuHandle.Offset(incrementSize * DescriptorHeapLayout::Depth),
		cpuHandle.Offset(incrementSize * DescriptorHeapLayout::Depth),
		m_DepthBuffer.Get(), (FLOAT*)&m_DepthClearValue, 0, nullptr);

	auto pPipeline = m_pDX12->GetPipeline();
	auto pComList = pPipeline->commandList;
	//set root sign and parameters
	pComList->SetComputeRootSignature(m_RootSignature.Get());

	ID3D12DescriptorHeap* descHeaps[]{ m_DescriptorHeapSV.Get() };
	pComList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	pComList->SetComputeRootConstantBufferView(1,m_AppDataBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(4, m_RasterizerBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(5, m_ScreenTileBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(6, m_RasterizerQBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootDescriptorTable(7, m_DescriptorHeapSV->GetGPUDescriptorHandleForHeapStart());
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
	m_AppData.tileDimensions = { 128,128 };
	m_AppData.quadricsPerRasterizer = 64;

	auto tileDim = GetNrTiles();
	UINT screenTiles = tileDim.height * tileDim.width;
	UINT extraRasterizers = screenTiles * 2;
	m_AppData.numRasterizers = screenTiles + extraRasterizers;

	InitResources();
	
	m_GPStage.Init(this);
	m_RStage.Init(this);
	m_MStage.Init(this);
}

void QuadricRenderer::Render()
{
	ThrowIfFailed(m_pDX12->GetDevice()->GetDeviceRemovedReason());

	InitRendering();

	auto pPipeline = m_pDX12->GetPipeline();
	auto pComList = pPipeline->commandList;

	for (QuadricGeometry* pGeo : m_ToRender)
	{
		InitDrawCall();
		if (m_GPStage.Execute(this, pGeo))
		{
			m_RStage.Execute(this);
			m_MStage.Execute(this);
		}
	}
	m_ToRender.clear();

	CopyToBackBuffer();
}

void QuadricRenderer::Render(const Instance& instance)
{
	auto pGeo = instance.GetGeometry();
	m_ToRender.insert(pGeo);
	pGeo->m_Transforms.push_back(instance.GetTransformMatrix());
}

void QuadricRenderer::Render(QuadricGeometry* pGeo, Transform& transform)
{
	transform.CalculateMatrix();
	Render(pGeo, transform.matrix);
}

void QuadricRenderer::Render(QuadricGeometry* pGeo, const DirectX::XMMATRIX& transform)
{
	m_ToRender.insert(pGeo);
	pGeo->m_Transforms.push_back(transform);
}
