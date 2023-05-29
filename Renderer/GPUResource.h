#pragma once
#include <cstdint>
#include "d3dx12.h"

class GPUResource
{
public:

	struct Descriptor
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandleSV;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandleSV;
		uint32_t indexSV;
		bool isActive;
	};

	enum class Type
	{
		None = 0,
		Buffer,
		Texture2D
	};

	CD3DX12_RESOURCE_BARRIER TransitionResource(D3D12_RESOURCE_STATES newState);

	virtual void* Map() = 0;
	virtual void Unmap(ID3D12GraphicsCommandList* pComList) = 0;

	Descriptor GetUAV() const { return m_UavDescriptor; }
	Descriptor GetSRV() const { return m_SrvDescriptor; }

	ID3D12Resource* Get() const { return m_Resource; }
	D3D12_RESOURCE_STATES GetCurrentState() const { return m_CurrentState; }
	virtual DXGI_FORMAT GetFormat() const = 0;
	Type GetType() const;

public:
	GPUResource() = default;

	GPUResource(GPUResource&&) noexcept;
	GPUResource(const GPUResource&) = delete;
	GPUResource& operator=(GPUResource&&) noexcept;
	GPUResource& operator=(const GPUResource&) = delete;
	virtual ~GPUResource();

protected:
	Descriptor m_UavDescriptor;
	Descriptor m_SrvDescriptor;

	ID3D12Resource* m_UploadResource;
	ID3D12Resource* m_Resource;

	D3D12_RESOURCE_STATES m_CurrentState;

	Type m_Type;
};