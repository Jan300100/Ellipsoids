#include "RasterizationStage.h"
#include "QuadricRenderer.h"
#include <d3dcompiler.h>
#include <array>

#ifndef USE_PIX
#define USE_PIX
#endif
#include <pix3.h>

using namespace Microsoft::WRL;

Stage::Rasterization::Rasterization()
	:Stage{}
{
}

void Stage::Rasterization::Init(QuadricRenderer* pRenderer)
{
	//SHADER
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(L"Shaders/RasterizationShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main", "cs_5_1", compileFlags, 0, &m_Shader, &errorBlob);

	if (errorBlob != nullptr)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);

#if defined(DEBUG) || defined(_DEBUG)  
	// pdb gen
	ID3DBlob* pdbBlob = nullptr;

	hr = D3DGetBlobPart(
		m_Shader->GetBufferPointer(),
		m_Shader->GetBufferSize(),
		D3D_BLOB_PDB,
		NULL,
		&pdbBlob
	);
	ThrowIfFailed(hr);

	FILE* pdbFile = nullptr;
	_wfopen_s(&pdbFile, L"RasterizationShader.pdb", L"wb");
	if (pdbFile)
	{
		fwrite(pdbBlob->GetBufferPointer(), 1, pdbBlob->GetBufferSize(), pdbFile);
		fclose(pdbFile);
	}
#endif

	//PSO
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc{};
	computePsoDesc.pRootSignature = pRenderer->m_RootSignature.Get();
	computePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(m_Shader->GetBufferPointer()),
		m_Shader->GetBufferSize()
	};
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(pRenderer->GetDevice()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_Pso)));

	m_Initialized = true;
}

void Stage::Rasterization::Execute(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList) const
{
	PIXScopedEvent(pComList, 0, "Stage::Rasterization");

	if (!m_Initialized) throw L"RasterizatonStage not initialized";

	//these are used as input here
	std::array<CD3DX12_RESOURCE_BARRIER, 4> barriers;
	barriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerIBuffer.Get());
	barriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerDepthBuffer.Get());
	barriers[2] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerBuffer.Get());
	barriers[3] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerQBuffer.Get());

	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());

	pComList->SetPipelineState(m_Pso.Get());

	UINT tileHeight = pRenderer->m_AppData.tileDimensions.height;
	pComList->Dispatch((tileHeight / 32) + ((tileHeight % 32) > 0), pRenderer->m_AppData.numRasterizers, 1);
}
