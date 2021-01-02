#include "MergeStage.h"
#include "QuadricRenderer.h"
#include <d3dcompiler.h>
#include "DX12.h"

using namespace Microsoft::WRL;

Stage::Merge::Merge(DX12* pDX12)
	:Stage{pDX12}
{
	
}

void Stage::Merge::Execute(QuadricRenderer* pRenderer) const
{
	DX12::Pipeline* pPipeline = m_pDX12->GetPipeline();
	auto pComList = pPipeline->commandList;

	//these are used as input here
	std::vector< CD3DX12_RESOURCE_BARRIER> barriers{};
	for (UINT i = 0; i < GBUFFER::NumBuffers; i++)
	{
		barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerGBuffers[i].Get()));
	}
	barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_OutputTexture.Get()));
	barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_DepthTexture.Get()));
	barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerBuffer.Get()));
	barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_ScreenTileBuffer.Get()));

	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());

	pComList->SetPipelineState(m_Pso.Get());
	pComList->SetComputeRootSignature(m_RootSignature.Get());

	ID3D12DescriptorHeap* descHeaps[]{  m_DescriptorHeap.Get() };
	pComList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	pComList->SetComputeRootConstantBufferView(0, pRenderer->m_AppDataBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootShaderResourceView(1, pRenderer->m_RasterizerBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootShaderResourceView(2, pRenderer->m_ScreenTileBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootDescriptorTable(3, m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	UINT threadGroupsVer = pRenderer->m_AppData.tileDimensions.height;
	UINT threadGroupsHor = pRenderer->m_AppData.tileDimensions.width;
	threadGroupsVer = (threadGroupsVer / 8) + ((threadGroupsVer % 32) > 0);
	threadGroupsHor = (threadGroupsHor / 8) + ((threadGroupsHor % 32) > 0);
	UINT numScreentiles = pRenderer->GetNrTiles().width * pRenderer->GetNrTiles().height;
	pComList->Dispatch(threadGroupsHor, threadGroupsVer, numScreentiles);


	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());

}

void Stage::Merge::Init(QuadricRenderer* pRenderer)
{
	//ROOT SIGNATURE
	CD3DX12_ROOT_PARAMETER rootParameter[4];

	CD3DX12_DESCRIPTOR_RANGE ranges[2]{};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 2, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	rootParameter[0].InitAsConstantBufferView(0);
	rootParameter[1].InitAsShaderResourceView(0);
	rootParameter[2].InitAsShaderResourceView(1);
	rootParameter[3].InitAsDescriptorTable(2, ranges);
	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, rootParameter,
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


	//SHADER
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	hr = D3DCompileFromFile(L"MergeShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
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
	ThrowIfFailed(m_pDX12->GetDevice()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_Pso)));


	//DESCRIPTOR HEAP
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_pDX12->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));
	UINT incrementSize = m_pDX12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//DESCRIPTORHEAP
	//descriptors
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	m_pDX12->GetDevice()->CreateShaderResourceView(pRenderer->m_RasterizerGBuffers[GBUFFER::Color].Get(), &srvDesc, srvHeapHandle);
	srvHeapHandle.Offset(incrementSize);
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	m_pDX12->GetDevice()->CreateShaderResourceView(pRenderer->m_RasterizerGBuffers[GBUFFER::Depth].Get(), &srvDesc, srvHeapHandle);
	srvHeapHandle.Offset(incrementSize);
	m_pDX12->GetDevice()->CreateUnorderedAccessView(pRenderer->m_OutputTexture.Get(), nullptr, &uavDesc, srvHeapHandle);
	srvHeapHandle.Offset(incrementSize);
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	m_pDX12->GetDevice()->CreateUnorderedAccessView(pRenderer->m_DepthTexture.Get(), nullptr, &uavDesc, srvHeapHandle);

}
