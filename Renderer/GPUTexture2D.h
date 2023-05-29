#pragma once
#include "GPUResource.h"

class GPUTexture2D : public GPUResource
{
public:
	struct Params
	{
		DXGI_FORMAT format;
		uint32_t width;
		uint32_t height;
		uint16_t numMips;
		bool allowUAV;
	};

	Params GetParams() const { return m_Params; }

	virtual DXGI_FORMAT GetFormat() const override { return m_Params.format; }

	virtual void* Map() override { return nullptr; }
	virtual void Unmap(ID3D12GraphicsCommandList*) override {}

public:
	GPUTexture2D() = default;
	GPUTexture2D(ID3D12Device* pDevice, const Params& params);
	GPUTexture2D(GPUTexture2D&&) noexcept;
	GPUTexture2D(const GPUTexture2D&) = delete;
	GPUTexture2D& operator=(GPUTexture2D&&) noexcept;
	GPUTexture2D& operator=(const GPUTexture2D&) = delete;
	virtual ~GPUTexture2D();

private:
	Params m_Params;
};