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
	//per mesh data : //Processed Shader results needed on the cpu, per mesh
	Microsoft::WRL::ComPtr<ID3D12Resource> m_MeshOutputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_MeshOutputResetBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_MeshOutputReadBackBuffer = nullptr;

	Transform m_Transform;
	void UpdateMeshData() const;

	MeshOutputData m_OutputData;

public:
	QuadricMesh(DX12* pDX12, const std::vector<InQuadric>& quadrics, const Transform& transform = {});
	MeshOutputData GetMeshOutput() const { return m_OutputData; };
	void ReadMeshOutput();
	ID3D12Resource* GetInputBuffer() const { return m_InputBuffer.Get(); }
	ID3D12Resource* GetMeshOutputBuffer() const { return m_MeshOutputBuffer.Get(); }
	ID3D12Resource* GetMeshOutputResetBuffer() const { return m_MeshOutputResetBuffer.Get(); }
	ID3D12Resource* GetMeshOutputReadbackBuffer() const { return m_MeshOutputReadBackBuffer.Get(); }
	ID3D12Resource* GetMeshDataBuffer() const { UpdateMeshData();  return m_MeshDataBuffer.Get(); }
	UINT QuadricsAmount() const { return (UINT)m_Quadrics.size(); }
	void UpdateBuffers(DX12* pDX12);
	Transform& GetTransform() { return m_Transform; }
};