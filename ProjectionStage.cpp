#include "ProjectionStage.h"
#include "Helpers.h"
#include <d3dcompiler.h>
#include "DX12.h"
#include "QuadricMesh.h"
#include "QuadricRenderer.h"

using namespace Microsoft::WRL;

Stage::Projection::Projection(DX12* pDX12)
	:Stage{pDX12}
{
	//ROOT SIGNATURE
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0);
	slotRootParameter[3].InitAsUnorderedAccessView(0);
	slotRootParameter[4].InitAsUnorderedAccessView(1);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
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

	hr = D3DCompileFromFile(L"ProjectionShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
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

void Stage::Projection::Execute(QuadricRenderer* pRenderer, QuadricMesh* pMesh) const
{
	DX12::Pipeline* pPipeline = m_pDX12->GetPipeline();
	auto pComList = pPipeline->commandList;
	pComList->SetPipelineState(m_Pso.Get());
	pComList->SetComputeRootSignature(m_RootSignature.Get());
	pComList->SetComputeRootConstantBufferView(0, pRenderer->m_AppDataBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootConstantBufferView(1, pMesh->GetMeshDataBuffer()->GetGPUVirtualAddress());
	pComList->SetComputeRootShaderResourceView(2, pMesh->GetInputBuffer()->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(3, pMesh->GetProjectedBuffer()->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(4, pRenderer->m_ScreenTileBuffer->GetGPUVirtualAddress());

	pComList->Dispatch((pMesh->QuadricsAmount() / 32) + 1 * ((pMesh->QuadricsAmount() % 32) > 0), 1, 1);

	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(pMesh->GetProjectedBuffer());
	pComList->ResourceBarrier(1, &barrier);
}

//
//void Stage::Projection::Project(ComPtr<ID3D12Resource> appDataBuffer, const std::vector<QuadricMesh*> meshes) const
//{
//	DX12::Pipeline* pPipeline = m_pDX12->GetPipeline();
//	auto pComList = pPipeline->commandList;
//
//
//	//DO THIS ALL AT ONCE IN QUADRIC RENDERER
//
//	////reset shaderoutput
//	std::vector<D3D12_RESOURCE_BARRIER> barriers{};
//	for (QuadricMesh* pQMesh : meshes)
//	{
//		pComList->CopyResource(pQMesh->GetShaderOutputBuffer(), pQMesh->GetShaderOutputUploadBuffer());
//		barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(pQMesh->GetShaderOutputBuffer(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
//	}
//	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());
//
//
//	//SET PSO and ROOT SIGN
//	pComList->SetPipelineState(m_Pso.Get());
//	pComList->SetComputeRootSignature(m_RootSignature.Get());
//	pComList->SetComputeRootConstantBufferView(0, appDataBuffer->GetGPUVirtualAddress());
//
//	//DISPATCH
//	for (QuadricMesh* pQMesh : meshes)
//	{
//		//Set root variables : meshdata, input buffer and outputbuffer
//		pComList->SetComputeRootConstantBufferView(1, pQMesh->GetMeshDataBuffer()->GetGPUVirtualAddress());
//		pComList->SetComputeRootShaderResourceView(2, pQMesh->GetInputBuffer()->GetGPUVirtualAddress());
//		pComList->SetComputeRootUnorderedAccessView(3, pQMesh->GetProjectedBuffer()->GetGPUVirtualAddress());
//		pComList->SetComputeRootUnorderedAccessView(4, pQMesh->GetShaderOutputBuffer()->GetGPUVirtualAddress());
//		pComList->Dispatch((pQMesh->QuadricsAmount() / 32) + 1 * ((pQMesh->QuadricsAmount() % 32) > 0), 1, 1); //these are the thread groups
//	}
//
//	//make sure they are all finished before continuing
//	barriers.clear();
//	for (QuadricMesh* pQMesh : meshes)
//	{
//		barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(pQMesh->GetProjectedBuffer()));
//		barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(pQMesh->GetShaderOutputBuffer()));
//		barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(pQMesh->GetShaderOutputBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
//	}
//	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());
//
//	barriers.clear();
//	for (QuadricMesh* pQMesh : meshes)
//	{
//		pComList->CopyResource(pQMesh->GetShaderOutputReadbackBuffer(), pQMesh->GetShaderOutputBuffer());
//		barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(pQMesh->GetShaderOutputBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
//	}
//	pComList->ResourceBarrier((UINT)barriers.size(), barriers.data());
//}
