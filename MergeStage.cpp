#include "MergeStage.h"
#include "QuadricRenderer.h"
#include <d3dcompiler.h>
#include "DX12.h"

using namespace Microsoft::WRL;

Stage::Merge::Merge(DX12* pDX12)
	:Stage{pDX12}
{
	//ROOT SIGNATURE
	CD3DX12_ROOT_PARAMETER rootParameter[4];

	CD3DX12_DESCRIPTOR_RANGE ranges[2];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 2, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

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
	ThrowIfFailed(pDX12->GetDevice()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_Pso)));
}

void Stage::Merge::Execute(QuadricRenderer*) const
{
}
