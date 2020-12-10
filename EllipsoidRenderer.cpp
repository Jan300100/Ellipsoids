#include "EllipsoidRenderer.h"
#include "DX12.h"
#include "Helpers.h"
#include "d3dx12.h"
#include <cmath>
#include "Window.h"
#include <d3dcompiler.h>
#include <iostream>
#include "Camera.h"


using namespace DirectX;

EllipsoidRenderer::EllipsoidRenderer(DX12* pDX12, Camera* pCamera)
	:m_pDX12{ pDX12 }, m_pCamera{pCamera}
{
	//pDX12->GetPipeline()->Flush();

	//Constant Buffer
	//**************
	auto properties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
	auto desc{ CD3DX12_RESOURCE_DESC::Buffer((sizeof(OutEllipsoid) + 255) & ~255)};
	m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_InputEllipsoidBuffer));

	//Constant Buffer
//**************
	properties = { CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
	desc  ={ CD3DX12_RESOURCE_DESC::Buffer((sizeof(FrameData) + 255) & ~255) };
	m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_InputDataBuffer));

	//Output Texture
	//***********
	D3D12_RESOURCE_DESC texDesc;

	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = pDX12->GetWindow()->GetDimensions().width;
	texDesc.Height = pDX12->GetWindow()->GetDimensions().height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_OutputTexture)));


	//descriptors
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	//DESCRIPTOR HEAP
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(pDX12->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

	pDX12->GetDevice()->CreateUnorderedAccessView(m_OutputTexture.Get(), nullptr, &uavDesc, m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//ROOT SIGNATURE
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	CD3DX12_DESCRIPTOR_RANGE range;
	range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1,0);
	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsDescriptorTable(1, &range);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(pDX12->GetDevice()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_RootSignature.GetAddressOf())));



	//SHADER
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	hr = D3DCompileFromFile(L"ellipsoid_rasterization.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main", "cs_5_1", compileFlags, 0, &m_Shader, &errorBlob);

	if (errorBlob != nullptr)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);


	//PSO
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc{};
	computePsoDesc.pRootSignature = m_RootSignature.Get();
	computePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(m_Shader->GetBufferPointer()),
		m_Shader->GetBufferSize()
	};
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(pDX12->GetDevice()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_Pso)));
}

void EllipsoidRenderer::RenderStart()
{
	DX12::Pipeline* pPipeline = m_pDX12->GetPipeline();

	ThrowIfFailed(pPipeline->commandAllocator->Reset());
	ThrowIfFailed(pPipeline->commandList->Reset(pPipeline->commandAllocator.Get(), m_Pso.Get()));

	// Indicate a state transition on the resource usage.
	auto transition{ CD3DX12_RESOURCE_BARRIER::Transition(pPipeline->GetCurrentRenderTarget(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET) };
	pPipeline->commandList->ResourceBarrier(1, &transition);

	// Clear the back buffer
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		pPipeline->rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		pPipeline->currentRT,
		m_pDX12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

	FLOAT col[4]{ 255,0,0,1 };
	pPipeline->commandList->ClearRenderTargetView(handle, col, 0, nullptr);

	// Specify the buffers to render to.
	pPipeline->commandList->OMSetRenderTargets(1, &handle, true, nullptr);

	//
	//SETUP COMPUTE
	auto pComList = m_pDX12->GetPipeline()->commandList;

	pComList->SetComputeRootSignature(m_RootSignature.Get());

	ID3D12DescriptorHeap* descHeaps[]{ m_DescriptorHeap.Get() };
	pComList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	FrameData data{};
	data.windowSize = { (float)m_pDX12->GetWindow()->GetDimensions().width ,(float)m_pDX12->GetWindow()->GetDimensions().height };
	data.lightDirection = { 0.577f,-0.577f,0.577f };

	//update input data
	BYTE* mapped = nullptr;
	m_InputDataBuffer->Map(0, nullptr,
		reinterpret_cast<void**>(&mapped));
	memcpy(mapped, &data, sizeof(FrameData));
	if (m_InputDataBuffer != nullptr)
		m_InputDataBuffer->Unmap(0, nullptr);

	pComList->SetComputeRootConstantBufferView(1, m_InputDataBuffer->GetGPUVirtualAddress());

	pComList->SetComputeRootDescriptorTable(2, m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

}

void EllipsoidRenderer::RenderFinish()
{
	DX12::Pipeline* pPipeline = m_pDX12->GetPipeline();

	//BIND COMPUTE BUFFER AS RENDERTARGET BACKBUFFER

	//COPY
	// Prepare to copy blurred output to the back buffer.
	auto transition{ CD3DX12_RESOURCE_BARRIER::Transition(pPipeline->GetCurrentRenderTarget(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST) };
	pPipeline->commandList->ResourceBarrier(1, &transition);

	pPipeline->commandList->CopyResource(pPipeline->GetCurrentRenderTarget(), m_OutputTexture.Get());

	// Transition to PRESENT state.
	transition = CD3DX12_RESOURCE_BARRIER::Transition(pPipeline->GetCurrentRenderTarget(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	pPipeline->commandList->ResourceBarrier(1, &transition);

	// Done recording commands.
	ThrowIfFailed(pPipeline->commandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { pPipeline->commandList.Get() };
	pPipeline->commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(pPipeline->swapChain->Present(0, 0));

	pPipeline->currentRT = (pPipeline->currentRT + 1) % pPipeline->rtvCount;

	pPipeline->Flush(); //wait for gpu to finish (== not ideal)
}

void EllipsoidRenderer::Render(const Ellipsoid& e)
{
	auto window = m_pDX12->GetWindow()->GetDimensions();
	
	OutEllipsoid result { e, m_pCamera };
	
	//rasterization on gpu
	//update input data
	BYTE* mapped = nullptr;
	m_InputEllipsoidBuffer->Map(0, nullptr,
		reinterpret_cast<void**>(&mapped));
	memcpy(mapped, &result, sizeof(OutEllipsoid));
	if (m_InputEllipsoidBuffer != nullptr)
		m_InputEllipsoidBuffer->Unmap(0, nullptr);

	auto pComList = m_pDX12->GetPipeline()->commandList;

	pComList->SetComputeRootConstantBufferView(0, m_InputEllipsoidBuffer->GetGPUVirtualAddress());
	pComList->Dispatch(window.width / 32 + 1, window.height / 32 + 1, 1); //these are the thread groups
	
}