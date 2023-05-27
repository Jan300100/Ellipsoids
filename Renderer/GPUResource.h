#pragma once
#include <cstdint>
#include "d3dx12.h"

class DeferredDeleteQueue;

class GPUResource
{
public:
	enum class Type
	{
		None = 0,
		Buffer,
		Texture2D
	};

	struct Texture2DParams
	{
		DXGI_FORMAT format;
		uint32_t width;
		uint32_t height;
		uint16_t numMips;
		bool allowUAV;
	};

	struct BufferParams {
		D3D12_HEAP_TYPE heapType;
		size_t size;
		bool allowUAV;
	};

	CD3DX12_RESOURCE_BARRIER TransitionResource(D3D12_RESOURCE_STATES newState);

	// mapping
	void* Map();
	void Unmap(ID3D12GraphicsCommandList* pComList);

	ID3D12Resource* Get() const { return m_Resource; }
	D3D12_RESOURCE_STATES GetCurrentState() const { return m_CurrentState; }
public:
	GPUResource();
	GPUResource(ID3D12Device* pDevice, const BufferParams& params);
	GPUResource(ID3D12Device* pDevice, const Texture2DParams& params);
	GPUResource(GPUResource&&) noexcept;
	GPUResource(const GPUResource&) = delete;
	GPUResource& operator=(GPUResource&&) noexcept;
	GPUResource& operator=(const GPUResource&) = delete;
	~GPUResource();

private:
	ID3D12Resource* m_UploadResource;
	ID3D12Resource* m_Resource;

	Type m_Type;
	BufferParams m_BufferParams;
	Texture2DParams m_Texture2DParams;

	D3D12_RESOURCE_STATES m_CurrentState;
};