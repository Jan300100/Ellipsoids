#pragma once
#include "Structs.h"
#include "Transform.h"
#include <vector>
#include <wrl.h>

class DX12;

class QuadricMesh
{
	std::vector<InQuadric> m_Quadrics; //data cpu
	Microsoft::WRL::ComPtr<ID3D12Resource> m_InputBuffer = nullptr; //has the cpu data
	Microsoft::WRL::ComPtr<ID3D12Resource> m_InputUploadBuffer = nullptr; //uploads the inputquadrics data to the gpu
	///PER MESH DATA : //contains the transformation matrix for the entire mesh
	Microsoft::WRL::ComPtr<ID3D12Resource> m_MeshDataBuffer = nullptr;

	Transform m_Transform;
	void UpdateMeshData() const;


public:
	QuadricMesh(DX12* pDX12, const std::vector<InQuadric>& quadrics, const Transform& transform = {});
	ID3D12Resource* GetInputBuffer() const { return m_InputBuffer.Get(); }
	ID3D12Resource* GetMeshDataBuffer() const { UpdateMeshData();  return m_MeshDataBuffer.Get(); }
	UINT QuadricsAmount() const { return (UINT)m_Quadrics.size(); }
	void UpdateBuffers(DX12* pDX12);
	Transform& GetTransform() { return m_Transform; }
};