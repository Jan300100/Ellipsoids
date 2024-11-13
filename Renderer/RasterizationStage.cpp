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
	// Shader
	ComPtr<IDxcUtils> pUtils;
	ComPtr<IDxcCompiler3> pCompiler;
	ComPtr<IDxcIncludeHandler> pIncludeHandler;
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));
	ThrowIfFailed(pUtils->CreateDefaultIncludeHandler(&pIncludeHandler));

	// Load the shader source file to a blob.
	ComPtr<IDxcBlobEncoding> sourceBlob;
	ThrowIfFailed(pUtils->LoadFile(L"Shaders/RasterizationShader.hlsl", nullptr, &sourceBlob));

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = 0u;

	std::vector<LPCWSTR> compilationArguments
	{
		L"-E",
		L"main",
		L"-T",
		L"cs_6_8",
		L"-I",
		L"Shaders/",
#if SHOW_TILES
		L"-D",
		L"SHOW_TILES",
		L"1",
#endif
#if !REVERSE_DEPTH
		L"-D",
		L"REVERSE_DEPTH",
		L"0",
#endif
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
	_wfopen_s(&pdbFile, L"RasterizationShaderDXC.pdb", L"wb");

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
