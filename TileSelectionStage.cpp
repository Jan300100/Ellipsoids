#include "TileSelectionStage.h"
#include "d3dx12.h"
#include "Helpers.h"
#include "DX12.h"
#include <d3dcompiler.h>
#include <vector>
#include "QuadricRenderer.h"


using namespace Microsoft::WRL;

Stage::TileSelection::TileSelection(DX12* pDX12)
	:Stage{pDX12}
{
	//Root sig
	CD3DX12_ROOT_PARAMETER parameters[4];
	parameters[0].InitAsConstantBufferView(0);
	parameters[1].InitAsUnorderedAccessView(0);
	parameters[2].InitAsUnorderedAccessView(1);
	parameters[3].InitAsUnorderedAccessView(2);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{ 4, parameters,0, nullptr };
	ComPtr<ID3DBlob> serialized = nullptr;
	ComPtr<ID3DBlob> errors = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serialized.GetAddressOf(), errors.GetAddressOf());

	if (errors)
	{
		::OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	ThrowIfFailed(pDX12->GetDevice()->CreateRootSignature(
		0,
		serialized->GetBufferPointer(),
		serialized->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature)));


	//SHADER
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	hr = D3DCompileFromFile(L"TileSelectionShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main", "cs_5_1", compileFlags, 0, &m_Shader, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());
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

	//BUFFERS
	auto byteSize = sizeof(Counters);
	auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_CountersResource)));

	//uploadBuffer
	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	ThrowIfFailed(m_pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_CountersUploadResource)));

	Counters initial{ 0, 0 };
	Counters* mapped = nullptr;
	m_CountersUploadResource->Map(0, nullptr,
		reinterpret_cast<void**>(&mapped));
	memcpy(mapped, &initial, byteSize);
	if (m_CountersUploadResource != nullptr)
		m_CountersUploadResource->Unmap(0, nullptr);

	//readback
	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK);
	desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_NONE);
	ThrowIfFailed(pDX12->GetDevice()->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_CountersReadbackResource)));
	m_CountersReadbackResource->SetName(LPCWSTR(L"m_CountersReadbackResource"));
	m_CountersUploadResource->SetName(LPCWSTR(L"m_CountersUploadResource"));
	m_CountersResource->SetName(LPCWSTR(L"m_CountersResource"));
}

void Stage::TileSelection::Execute(QuadricRenderer* pRenderer, QuadricMesh*) const
{
	DX12::Pipeline* pPipeline = m_pDX12->GetPipeline();
	auto pComList = pPipeline->commandList;

	pComList->CopyResource(m_CountersResource.Get(), m_CountersUploadResource.Get());

	CD3DX12_RESOURCE_BARRIER barrier;
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_CountersResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pComList->ResourceBarrier(1, &barrier);

	pComList->SetPipelineState(m_Pso.Get());
	pComList->SetComputeRootSignature(m_RootSignature.Get());
	pComList->SetComputeRootConstantBufferView(0, pRenderer->m_AppDataBuffer->GetGPUVirtualAddress());
	//pComList->SetComputeRootUnorderedAccessView(1, pRenderer->m_ScreenTileBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(2, pRenderer->m_RasterizerBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(3, m_CountersResource->GetGPUVirtualAddress());

	unsigned int horizontalTiles{ (m_pDX12->GetWindow()->GetDimensions().width / pRenderer->m_AppData.tileDimensions.width) + 1 }
		, verticalTiles{ (m_pDX12->GetWindow()->GetDimensions().height / pRenderer->m_AppData.tileDimensions.height) + 1 }
	, nrTiles{ horizontalTiles * verticalTiles };

	pComList->Dispatch((nrTiles / 32) + 1 * ((nrTiles % 32) > 0), 1, 1);

	CD3DX12_RESOURCE_BARRIER barriers[3];
	//barriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_ScreenTileBuffer.Get());
	barriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerBuffer.Get());
	barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_CountersResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);

	pComList->ResourceBarrier(3, barriers);
}