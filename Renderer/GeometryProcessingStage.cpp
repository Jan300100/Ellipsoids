#include "GeometryProcessingStage.h"
#include "QuadricGeometry.h"
#include "QuadricRenderer.h"
#include <d3dcompiler.h>
#include <array>

#ifndef USE_PIX
#define USE_PIX
#endif
#include <pix3.h>

using namespace Microsoft::WRL;

#define PROJECT_DIR_STR XSTR(PROJECT_DIR)
#define XSTR(s) STR(s)
#define STR(s) #s

Stage::GeometryProcessing::GeometryProcessing()
	:Stage{}
{
}

bool Stage::GeometryProcessing::Execute(QuadricRenderer*, ID3D12GraphicsCommandList* pComList, QuadricGeometry* pGeometry) const
{
	PIXScopedEvent(pComList, 0, "Stage::GeometryProcessing");
	if (!m_Initialized) throw L"GeometryProcessingStage not initialized";

	if (pGeometry->QuadricsAmount() == 0) return false;

	pComList->SetComputeRoot32BitConstant(0, (UINT)pGeometry->QuadricsAmount(), 0);
	pComList->SetComputeRootShaderResourceView(2, pGeometry->GetTransformBuffer()->GetGPUVirtualAddress());
	pComList->SetComputeRootShaderResourceView(3, pGeometry->GetInputBuffer()->GetGPUVirtualAddress());

	pComList->SetPipelineState(m_Pso.Get());
	pComList->Dispatch((UINT)(pGeometry->QuadricsAmount() / 32) + ((pGeometry->QuadricsAmount() % 32) > 0), 1, (UINT)pGeometry->GetNumInstances());
	return true;
}

void Stage::GeometryProcessing::Init(QuadricRenderer* pRenderer)
{
	//SHADER
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(L"Shaders/GeometryProcessingShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
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
	_wfopen_s(&pdbFile, L"GeometryProcessingShader.pdb", L"wb");
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
