#include "MergeStage.h"
#include "QuadricRenderer.h"
#include <d3dcompiler.h>
#include <array>

#ifndef USE_PIX
#define USE_PIX
#endif
#include <pix3.h>

using namespace Microsoft::WRL;

Stage::Merge::Merge()
	:Stage{}
{
	
}

void Stage::Merge::Execute(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList) const
{
	PIXScopedEvent(pComList, 0, "Stage::Merge");

	if (!m_Initialized) throw L"MergeState not initialized";

	//these are used as input here
	std::array< CD3DX12_RESOURCE_BARRIER, 6> barriers{};
	barriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerIBuffer.Get());
	barriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerDepthBuffer.Get());
	barriers[2] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_OutputBuffer.Get());
	barriers[3] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_DepthBuffer.Get());
	barriers[4] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerBuffer.Get());
	barriers[5] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_ScreenTileBuffer.Get());

	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());

	UINT threadGroupsVer = pRenderer->m_AppData.tileDimensions.height;
	UINT threadGroupsHor = pRenderer->m_AppData.tileDimensions.width;
	threadGroupsVer = (threadGroupsVer / 8) + ((threadGroupsVer % 32) > 0);
	threadGroupsHor = (threadGroupsHor / 8) + ((threadGroupsHor % 32) > 0);
	UINT numScreentiles = pRenderer->GetNrTiles().width * pRenderer->GetNrTiles().height;

	pComList->SetPipelineState(m_Pso.Get());

	pComList->Dispatch(threadGroupsHor, threadGroupsVer, numScreentiles);

	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());

}

void Stage::Merge::Init(QuadricRenderer* pRenderer)
{
	//SHADER
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else 
	compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(L"Shaders/MergeShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
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
	_wfopen_s(&pdbFile, L"MergeShader.pdb", L"wb");
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
