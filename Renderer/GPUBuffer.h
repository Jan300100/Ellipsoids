#pragma once
#include "GPUResource.h"

class GPUBuffer : public GPUResource
{
public:
	struct Params {
		D3D12_HEAP_TYPE heapType;
		uint32_t numElements;
		uint32_t elementSize;
		bool allowUAV;
	};

	Descriptor GetCBV() const { return m_CbvDescriptor; }

	Params GetParams() const { return m_Params; }

	uint32_t GetSize() const { return m_Size; }

	virtual DXGI_FORMAT GetFormat() const override { return DXGI_FORMAT_UNKNOWN; }

	virtual void* Map() override;
	virtual void Unmap(ID3D12GraphicsCommandList* pComList) override;

public:
	GPUBuffer() = default;
	GPUBuffer(ID3D12Device* pDevice, const Params& params);
	GPUBuffer(GPUBuffer&&) noexcept;
	GPUBuffer(const GPUBuffer&) = delete;
	GPUBuffer& operator=(GPUBuffer&&) noexcept;
	GPUBuffer& operator=(const GPUBuffer&) = delete;
	virtual ~GPUBuffer();

private:
	uint32_t m_Size;
	Params m_Params;
	Descriptor m_CbvDescriptor;
};