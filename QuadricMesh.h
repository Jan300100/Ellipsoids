#pragma once
#include "Quadric.h"
#include "Transform.h"
#include <vector>

class DX12;

struct MeshData
{
	DirectX::XMMATRIX transform;
	unsigned int numPatches = 0;
};

class QuadricMesh
{
	std::vector<InQuadric> m_Quadrics; //data cpu
	ComPtr<ID3D12Resource> m_InputBuffer = nullptr; //has the cpu data
	ComPtr<ID3D12Resource> m_InputUploadBuffer = nullptr; //uploads data from cpu
	ComPtr<ID3D12Resource> m_OutputProjectedBuffer = nullptr; //output of projection shader
	
	///PER MESH DATA : //contains the transformation matrix for the entire mesh
	ComPtr<ID3D12Resource> m_MeshDataBuffer = nullptr; 
	Transform m_Transform;
	void UpdateMeshData();
public:
	QuadricMesh(DX12* pDX12, const std::vector<InQuadric>& quadrics, const Transform& transform = {});
	ID3D12Resource* GetInputBuffer() { return m_InputBuffer.Get(); }
	ID3D12Resource* GetProjectedBuffer() { return m_OutputProjectedBuffer.Get(); }
	ID3D12Resource* GetMeshDataBuffer() { UpdateMeshData();  return m_MeshDataBuffer.Get(); }
	UINT QuadricsAmount() { return (UINT)m_Quadrics.size(); }
	void UpdateBuffers(DX12* pDX12);
	Transform& GetTransform() { return m_Transform; }
};