#include "UploadManager.h"
#include "Helpers.h"

UploadManager::UploadManager(ID3D12Device2* pDevice)
{
	CD3DX12_HEAP_PROPERTIES properties = { CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };

	D3D12_HEAP_DESC heapDesc{};
	heapDesc.SizeInBytes = 64 * 1024 * 1024; // 64MB
	heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapDesc.Flags = D3D12_HEAP_FLAG_NONE;
	heapDesc.Properties = properties;

	HRESULT hr = pDevice->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_pHeap));
	
	ThrowIfFailed(hr);
	
	m_pHeap->SetName(L"UploadHeap");
}

void* UploadManager::Map(ID3D12Resource* pResource)
{
	// create temporary uploadobject
	return nullptr;
}

void UploadManager::UnMap(ID3D12Resource* pResource)
{
	// issue copy command and 
	// destroy temporary object after.
}
