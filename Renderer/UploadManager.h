#pragma once
#include "d3dx12.h"
#include <wrl.h>


class UploadManager
{
public:
	UploadManager(ID3D12Device2* pDevice);
	void* Map(ID3D12Resource* pResource);
	void UnMap(ID3D12Resource* pResource);
private:
	Microsoft::WRL::ComPtr<ID3D12Heap> m_pHeap;
};