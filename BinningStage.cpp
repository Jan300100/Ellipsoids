#include "BinningStage.h"
#include "d3dx12.h"
#include "Helpers.h"
#include <d3dcompiler.h>
#include "DX12.h"
#include "QuadricMesh.h"
#include "QuadricRenderer.h"

using namespace Microsoft::WRL;

Stage::Binning::Binning(DX12* pDX12)
	: Stage{pDX12} 
{
	//ROOT SIGNATURE
	CD3DX12_ROOT_PARAMETER slotRootParameter[6];

	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0);
	slotRootParameter[3].InitAsShaderResourceView(1);
	slotRootParameter[4].InitAsUnorderedAccessView(0);
	slotRootParameter[5].InitAsUnorderedAccessView(1);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(6, slotRootParameter,
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

	hr = D3DCompileFromFile(L"BinningShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
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

void Stage::Binning::Execute(QuadricRenderer* pRenderer, QuadricMesh* pMesh) const
{
	DX12::Pipeline* pPipeline = m_pDX12->GetPipeline();
	auto pComList = pPipeline->commandList;

	pComList->SetPipelineState(m_Pso.Get());
	pComList->SetComputeRootSignature(m_RootSignature.Get());

	pComList->SetComputeRootConstantBufferView(0, pRenderer->m_AppDataBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootConstantBufferView(1, pMesh->GetMeshDataBuffer()->GetGPUVirtualAddress());
	pComList->SetComputeRootShaderResourceView(2, pMesh->GetProjectedBuffer()->GetGPUVirtualAddress());
	//pComList->SetComputeRootShaderResourceView(3, pRenderer->m_ScreenTileBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(4, pRenderer->m_RasterizerQBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(5, pRenderer->m_RasterizerBuffer->GetGPUVirtualAddress());
	
	pComList->Dispatch((pMesh->QuadricsAmount() / 32) + 1 * ((pMesh->QuadricsAmount() % 32) > 0), 1, 1);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(pRenderer->m_RasterizerQBuffer.Get());
	pComList->ResourceBarrier(1, &barrier);
}
