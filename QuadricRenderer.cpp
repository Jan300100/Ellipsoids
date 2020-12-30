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

	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_DepthTexture)));

	//descriptors
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	//DESCRIPTOR HEAP
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	
	srvHeapDesc.NumDescriptors = 2;
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

	//init resources
	auto pPipeline = m_pDX12->GetPipeline();
	ThrowIfFailed(pPipeline->commandAllocator->Reset());
	ThrowIfFailed(pPipeline->commandList->Reset(pPipeline->commandAllocator.Get(), nullptr));

	CD3DX12_RESOURCE_BARRIER transitions[2]
	{
		CD3DX12_RESOURCE_BARRIER::Transition(m_OutputTexture.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(m_DepthTexture.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	};
	pPipeline->commandList->ResourceBarrier(2, transitions);

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

QuadricRenderer::QuadricRenderer(DX12* pDX12, Camera* pCamera)
	:m_pDX12{ pDX12 }, m_pCamera{ pCamera }, m_AppData{}, m_ProjStage{pDX12}
{
	m_AppData.windowSize = { m_pDX12->GetWindow()->GetDimensions().width ,m_pDX12->GetWindow()->GetDimensions().height, 0, 0 };
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

	m_ProjStage.Project(m_AppDataBuffer, m_ToRender);
	CopyToBackBuffer();

	//reset for next frame
	m_ToRender.clear();
}

void QuadricRenderer::Render(QuadricMesh* pMesh)
{
	//registers the quadric for rendering
	m_ToRender.push_back(pMesh);
}
