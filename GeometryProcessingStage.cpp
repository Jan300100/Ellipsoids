#include "GeometryProcessingStage.h"
#include "QuadricMesh.h"
#include "QuadricRenderer.h"
#include "DX12.h"
#include <d3dcompiler.h>

using namespace Microsoft::WRL;


Stage::GeometryProcessing::GeometryProcessing(DX12* pDX12)
	:Stage{pDX12}
{
	
}

bool Stage::GeometryProcessing::Execute(QuadricRenderer* pRenderer, QuadricMesh* pMesh) const
{
	DX12::Pipeline* pPipeline = m_pDX12->GetPipeline();
	auto pComList = pPipeline->commandList;

	std::vector< CD3DX12_RESOURCE_BARRIER> barriers{};
	barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerBuffer.Get()));
	barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerQBuffer.Get()));
	barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_ScreenTileBuffer.Get()));
	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());
	pComList->SetComputeRoot32BitConstant(0, pMesh->QuadricsAmount(), 0);
	pComList->SetComputeRootShaderResourceView(2, pMesh->GetTransformBuffer()->GetGPUVirtualAddress());
	pComList->SetComputeRootShaderResourceView(3, pMesh->GetInputBuffer()->GetGPUVirtualAddress());

	pComList->SetPipelineState(m_Pso.Get());

	UINT amount = pMesh->UpdateTransforms();
	if (amount == 0) return false;
	pComList->Dispatch((pMesh->QuadricsAmount() / 32) + ((pMesh->QuadricsAmount() % 32) > 0), 1, amount);
	return true;
}

void Stage::GeometryProcessing::Init(QuadricRenderer* pRenderer)
{

	//SHADER
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(L"GeometryProcessingShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main", "cs_5_1", compileFlags, 0, &m_Shader, &errorBlob);

	if (errorBlob != nullptr)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);

	//PSO
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc{};
	computePsoDesc.pRootSignature = pRenderer->m_RootSignature.Get();
	computePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(m_Shader->GetBufferPointer()),
		m_Shader->GetBufferSize()
	};
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(m_pDX12->GetDevice()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_Pso)));
}
