#include "DescriptorManager.h"
#include "Helpers.h"

DescriptorManager DescriptorManager::s_Instance;

DescriptorManager::~DescriptorManager()
{
	if (m_pDescriptorHeap)
	{
		m_pDescriptorHeap->Release();
	}

	if (m_pDescriptorHeapSV)
	{
		m_pDescriptorHeapSV->Release();
	}
}

DescriptorManager* DescriptorManager::Instance()
{
	return &s_Instance;
}

void DescriptorManager::Initialize(ID3D12Device* pDevice)
{
	if (!m_Initialized)
	{
		m_Initialized = true;

		m_pDevice = pDevice;
		m_IncrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_NumDescriptors = 128;

		//DESCRIPTOR HEAP, todo: make more dynamic
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = m_NumDescriptors;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pDescriptorHeapSV)));

		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pDescriptorHeap)));

		m_Next.cpuHandle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_Next.cpuHandleSV = m_pDescriptorHeapSV->GetCPUDescriptorHandleForHeapStart();
		m_Next.gpuHandleSV = m_pDescriptorHeapSV->GetGPUDescriptorHandleForHeapStart();
		m_Next.indexSV = 0;
		m_Next.isActive = false;
	}
}

GPUResource::Descriptor DescriptorManager::CreateUAV(const GPUResource& resource)
{
	GPUResource::Descriptor data;

	if (!m_FreeList.empty())
	{
		data = m_FreeList.back();
		m_FreeList.pop_back();
	}
	else
	{
		data = m_Next;

		m_Next.cpuHandle.Offset(m_IncrementSize);
		m_Next.cpuHandleSV.Offset(m_IncrementSize);
		m_Next.gpuHandleSV.Offset(m_IncrementSize);
		m_Next.indexSV += 1;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
	D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc = nullptr;
	switch (resource.GetType())
	{
	case GPUResource::Type::None:
		break;
	case GPUResource::Type::Buffer:
	{
		GPUResource::BufferParams params = resource.GetBufferParams();
		desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		desc.Buffer.StructureByteStride = params.elementSize;
		desc.Buffer.NumElements = params.numElements;
		pDesc = &desc;
	}
		break;
	case GPUResource::Type::Texture2D:
		break;
	default:
		break;
	}

	m_pDevice->CreateUnorderedAccessView(resource.Get(), nullptr, pDesc, data.cpuHandle);
	m_pDevice->CreateUnorderedAccessView(resource.Get(), nullptr, pDesc, data.cpuHandleSV);
	data.isActive = true;

	return data;
}

GPUResource::Descriptor DescriptorManager::CreateSRV(const GPUResource& resource)
{
	GPUResource::Descriptor data;

	if (!m_FreeList.empty())
	{
		data = m_FreeList.back();
		m_FreeList.pop_back();
	}
	else
	{
		data = m_Next;

		m_Next.cpuHandle.Offset(m_IncrementSize);
		m_Next.cpuHandleSV.Offset(m_IncrementSize);
		m_Next.gpuHandleSV.Offset(m_IncrementSize);
		m_Next.indexSV += 1;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
	D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc = nullptr;
	switch (resource.GetType())
	{
	case GPUResource::Type::None:
		break;
	case GPUResource::Type::Buffer:
	{
		GPUResource::BufferParams params = resource.GetBufferParams();
		desc.Format = resource.GetFormat();
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; //required for structured buffer
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Buffer.StructureByteStride = params.elementSize;
		desc.Buffer.NumElements = params.numElements;
		pDesc = &desc;
	}
	break;
	case GPUResource::Type::Texture2D:
		break;
	default:
		break;
	}
	
	m_pDevice->CreateShaderResourceView(resource.Get(), pDesc, data.cpuHandle);
	m_pDevice->CreateShaderResourceView(resource.Get(), pDesc, data.cpuHandleSV);

	data.isActive = true;

	return data;
}

void DescriptorManager::Free(const GPUResource::Descriptor& data)
{
	if (data.isActive)
	{
		m_FreeList.emplace_back(data);
		m_FreeList.back().isActive = false;
	}
}

D3D12_UAV_DIMENSION DescriptorManager::GetUAVViewDimension(GPUResource::Type type)
{
	D3D12_UAV_DIMENSION viewDim = D3D12_UAV_DIMENSION_UNKNOWN;
	switch (type)
	{
	case GPUResource::Type::None:
		break;
	case GPUResource::Type::Buffer:
		viewDim = D3D12_UAV_DIMENSION_BUFFER;
		break;
	case GPUResource::Type::Texture2D:
		viewDim = D3D12_UAV_DIMENSION_TEXTURE2D;
		break;
	default:
		break;
	}
	return viewDim;
}

D3D12_SRV_DIMENSION DescriptorManager::GetSRVViewDimension(GPUResource::Type type)
{
	D3D12_SRV_DIMENSION viewDim = D3D12_SRV_DIMENSION_UNKNOWN;
	switch (type)
	{
	case GPUResource::Type::None:
		break;
	case GPUResource::Type::Buffer:
		viewDim = D3D12_SRV_DIMENSION_BUFFER;
		break;
	case GPUResource::Type::Texture2D:
		viewDim = D3D12_SRV_DIMENSION_TEXTURE2D;
		break;
	default:
		break;
	}
	return viewDim;
}
