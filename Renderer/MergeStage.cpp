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
	std::array< CD3DX12_RESOURCE_BARRIER, 2> barriers{};
	barriers[2] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_OutputBuffer.Get());
	barriers[3] = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_DepthBuffer.Get());

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
	// Shader
	ComPtr<IDxcUtils> pUtils;
	ComPtr<IDxcCompiler3> pCompiler;
	ComPtr<IDxcIncludeHandler> pIncludeHandler;
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));
	ThrowIfFailed(pUtils->CreateDefaultIncludeHandler(&pIncludeHandler));

	// Load the shader source file to a blob.
	ComPtr<IDxcBlobEncoding> sourceBlob;
	ThrowIfFailed(pUtils->LoadFile(L"Shaders/MergeShader.hlsl", nullptr, &sourceBlob));

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = 0u;

	std::vector<LPCWSTR> compilationArguments
	{
		L"-E",
		L"main",
		L"-T",
		L"cs_6_6",
		L"-I",
		L"Shaders/",
	};

	compilationArguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);

#if defined(DEBUG) || defined(_DEBUG)  
	compilationArguments.push_back(DXC_ARG_DEBUG);
	compilationArguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#else
	compilationArguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

	// Compile the shader.
	Microsoft::WRL::ComPtr<IDxcResult> compiledShaderBuffer{};
	HRESULT hr = pCompiler->Compile(&sourceBuffer,
		compilationArguments.data(),
		static_cast<uint32_t>(compilationArguments.size()),
		pIncludeHandler.Get(),
		IID_PPV_ARGS(&compiledShaderBuffer));

	ThrowIfFailed(hr);

	// Get compilation errors (if any).
	ComPtr<IDxcBlobEncoding> errors{};
	hr = compiledShaderBuffer->GetErrorBuffer(errors.GetAddressOf());
	ThrowIfFailed(hr);
	if (errors)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}

#if defined(DEBUG) || defined(_DEBUG)  
	// save pdbs
	ComPtr<IDxcBlob> pdbBlob{};
	compiledShaderBuffer->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdbBlob), nullptr);
	FILE* pdbFile = nullptr;
	_wfopen_s(&pdbFile, L"MergeShaderDXC.pdb", L"wb");
	if (pdbBlob && pdbFile)
	{
		fwrite(pdbBlob->GetBufferPointer(), 1, pdbBlob->GetBufferSize(), pdbFile);
		fclose(pdbFile);
	}
#endif

	compiledShaderBuffer->GetResult(m_Shader.GetAddressOf());

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
