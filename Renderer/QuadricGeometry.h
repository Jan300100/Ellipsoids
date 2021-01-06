#pragma once
#include "Structs.h"
#include "Transform.h"
#include <vector>
#include <wrl.h>
#include <dxgi1_6.h>


class QuadricGeometry
{
	friend class QuadricRenderer;
	//data cpu
	UINT m_MaxInstances;
	std::vector<InQuadric> m_Quadrics; 
	std::vector<DirectX::XMMATRIX> m_Transforms; 
	Microsoft::WRL::ComPtr<ID3D12Resource> m_InputBuffer = nullptr; //has the cpu data
	Microsoft::WRL::ComPtr<ID3D12Resource> m_InputUploadBuffer = nullptr; //uploads the inputquadrics data to the gpu
	///PER MESH DATA : //contains the transformation matrix for the entire mesh
	Microsoft::WRL::ComPtr<ID3D12Resource> m_MeshDataBuffer = nullptr;
public:
	QuadricGeometry(ID3D12Device2* pDevice, ID3D12GraphicsCommandList* pComList, const std::vector<InQuadric>& quadrics, UINT maxInstances = 1);
	ID3D12Resource* GetInputBuffer() const { return m_InputBuffer.Get(); }
	ID3D12Resource* GetTransformBuffer() { return m_MeshDataBuffer.Get(); }
	UINT UpdateTransforms();

	UINT QuadricsAmount() const { return (UINT)m_Quadrics.size(); }
	void UpdateBuffers(ID3D12GraphicsCommandList* pComList);
};