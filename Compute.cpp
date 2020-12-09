#include "Compute.h"
#include "Helpers.h"
#include "Pipeline.h"
#include "d3dx12.h"
#include <vector>
#include <D3Dcompiler.h>
#include <iostream>
#include "Win32Window.h"

Compute::Compute(Pipeline* pPipeline, Win32Window* pWindow) : m_pPipeline{ pPipeline }, m_pWindow{pWindow}
{
	if (pPipeline)
	{

		auto pDev = pPipeline->GetDevice();
		auto pComList = pPipeline->GetCommandList();
		auto pComQueue = pPipeline->GetCommandQueue();
		auto pComAlloc = pPipeline->GetCommandAllocator();

		//reset list
		pComList->Reset(pComAlloc.Get(), nullptr);

		//Input Data

		std::vector<float> data{};
		for (size_t i = 1; i <= pWindow->GetDimensions().width; i++)
		{
			data.push_back(float(i) / pWindow->GetDimensions().width);
		}

		UINT64 byteSize = data.size() * sizeof(float);

		// Create the actual default buffer resource.
		ThrowIfFailed(pDev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(m_InputBuffer.GetAddressOf())));

		// In order to copy CPU memory data into our default buffer, we need to create
		// an intermediate upload heap. 
		ThrowIfFailed(pDev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_InputBufferUpload.GetAddressOf())));


		// Describe the data we want to copy into the default buffer.
		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = data.data();
		subResourceData.RowPitch = byteSize;
		subResourceData.SlicePitch = subResourceData.RowPitch;

		// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
		// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
		// the intermediate upload heap data will be copied to mBuffer.
		pComList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_InputBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		UpdateSubresources<1>(pComList.Get(), m_InputBuffer.Get(), m_InputBufferUpload.Get(), 0, 0, 1, &subResourceData);
		pComList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_InputBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));


		// Create the buffer that will be a UAV.
		ThrowIfFailed(pDev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&m_OutputBuffer)));

		ThrowIfFailed(pDev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_OutputBufferReadback)));


		//OUTPUT TEXTURE
		CreateOutputTexture();

		//ROOT SIGNATURE
		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[3];


		CD3DX12_DESCRIPTOR_RANGE range;
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsShaderResourceView(0);
		slotRootParameter[1].InitAsUnorderedAccessView(0);
		slotRootParameter[2].InitAsDescriptorTable(1, &range);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(pDev->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(m_RootSignature.GetAddressOf())));

		

		//SHADER
		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		hr = D3DCompileFromFile(L"demo.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"main", "cs_5_1", compileFlags, 0, &m_Shader, &errorBlob);

		if (errorBlob != nullptr)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		ThrowIfFailed(hr);


		//PSO
		D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
		computePsoDesc.pRootSignature = m_RootSignature.Get();
		computePsoDesc.CS =
		{
			reinterpret_cast<BYTE*>(m_Shader->GetBufferPointer()),
			m_Shader->GetBufferSize()
		};
		computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		ThrowIfFailed(pPipeline->GetDevice()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_Pso)));

		// Execute the initialization commands.
		ThrowIfFailed(pComList->Close());
		ID3D12CommandList* cmdsLists[] = { pComList.Get() };
		pComQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until initialization is complete.
		pPipeline->Flush();
	}
}

void Compute::CreateOutputTexture()
{
	D3D12_RESOURCE_DESC texDesc;
	
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_pWindow->GetDimensions().width;
	texDesc.Height = m_pWindow->GetDimensions().height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(m_pPipeline->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_OutputTexture)));


	//descriptors
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	//DESCRIPTOR HEAP
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_pPipeline->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

	m_pPipeline->GetDevice()->CreateUnorderedAccessView(m_OutputTexture.Get(), nullptr, &uavDesc, m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void Compute::Execute()
{
	auto pDev = m_pPipeline->GetDevice();
	auto pComList = m_pPipeline->GetCommandList();
	auto pComQueue = m_pPipeline->GetCommandQueue();
	auto pComAlloc = m_pPipeline->GetCommandAllocator();

	pComAlloc->Reset();
	pComList->Reset(pComAlloc.Get(), m_Pso.Get());

	pComList->SetComputeRootSignature(m_RootSignature.Get());

	ID3D12DescriptorHeap* descHeaps[]{ m_DescriptorHeap.Get() };
	pComList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
	
	pComList->SetComputeRootShaderResourceView(0, m_InputBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootUnorderedAccessView(1, m_OutputBuffer->GetGPUVirtualAddress());
	pComList->SetComputeRootDescriptorTable(2, m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	pComList->Dispatch(m_pWindow->GetDimensions().width / 32 + 1, m_pWindow->GetDimensions().height / 32 + 1, 1); //these are the thread groups

	// Schedule to copy the data to the default buffer to the readback buffer.

	pComList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_OutputBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	pComList->CopyResource(m_OutputBufferReadback.Get(), m_OutputBuffer.Get());

	pComList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_OutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	//Done recording commands
	ThrowIfFailed(pComList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { pComList.Get() };
	pComQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	m_pPipeline->Flush();

	// Map the data so we can read it on CPU.
	float* mappedData = nullptr;
	ThrowIfFailed(m_OutputBufferReadback->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

	//for (int i = 0; i < 100; ++i)
	//{
	//	std::cout << mappedData[i] << std::endl;
	//}

	m_OutputBufferReadback->Unmap(0, nullptr);

}
