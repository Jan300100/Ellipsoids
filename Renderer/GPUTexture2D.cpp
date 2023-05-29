#include "GPUTexture2D.h"
#include "Helpers.h"
#include "DescriptorManager.h"

GPUTexture2D::GPUTexture2D(ID3D12Device* pDevice, const Params& params)
	:GPUResource{}
	, m_Params{params}
{
	m_Type = GPUResource::Type::Texture2D;

	CD3DX12_HEAP_PROPERTIES properties{ D3D12_HEAP_TYPE_DEFAULT };
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(params.format, params.width, params.height);
	desc.MipLevels = params.numMips;

	if (params.allowUAV)
	{
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		m_CurrentState,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));

	if (m_Params.allowUAV)
	{
		m_UavDescriptor = DescriptorManager::Instance()->CreateUAV(*this);
	}
	m_SrvDescriptor = DescriptorManager::Instance()->CreateSRV(*this);
}

GPUTexture2D::GPUTexture2D(GPUTexture2D&& other) noexcept
	:GPUResource{std::move(other)}
	, m_Params{ other.m_Params }
{
	other.m_Params = {};
}

GPUTexture2D& GPUTexture2D::operator=(GPUTexture2D&& other) noexcept
{
	GPUResource::operator=(std::move(other));

	m_Params = other.m_Params;
	other.m_Params = {};

	return *this;
}

GPUTexture2D::~GPUTexture2D()
{
}
