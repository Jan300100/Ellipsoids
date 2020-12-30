#pragma once
#include "Quadric.h"
#include "Transform.h"
#include <vector>

class DX12;


struct ShaderOutput
{
	DirectX::XMUINT4 boundingBox; //in screenspace
	unsigned int numPatches;
};

struct MeshData
{
	DirectX::XMMATRIX transform; //in screenspace
	unsigned int numQuadrics;
};

class QuadricMesh
{
	std::vector<InQuadric> m_Quadrics; //data cpu
	ComPtr<ID3D12Resource> m_InputBuffer = nullptr; //has the cpu data
	ComPtr<ID3D12Resource> m_InputUploadBuffer = nullptr; //uploads the inputquadrics data to the gpu
	ComPtr<ID3D12Resource> m_OutputProjectedBuffer = nullptr; //output buffer of projection shader
	
	///PER MESH DATA : //contains the transformation matrix for the entire mesh
	ComPtr<ID3D12Resource> m_MeshDataBuffer = nullptr; 
	//per mesh data : //Shader results needed on the cpu, per mesh
	ComPtr<ID3D12Resource> m_ShaderOutputBuffer = nullptr;
	ComPtr<ID3D12Resource> m_ShaderOutputReadbackBuffer = nullptr;
	static ID3D12Resource* m_ShaderOutputUploadBuffer;
	static unsigned int m_NrMeshes;

	ShaderOutput m_Output;
	Transform m_Transform;
	void UpdateMeshData();
public:
	QuadricMesh(DX12* pDX12, const std::vector<InQuadric>& quadrics, const Transform& transform = {});
	~QuadricMesh();
	ID3D12Resource* GetInputBuffer() { return m_InputBuffer.Get(); }
	ID3D12Resource* GetProjectedBuffer() { return m_OutputProjectedBuffer.Get(); }
	ID3D12Resource* GetMeshDataBuffer() { UpdateMeshData();  return m_MeshDataBuffer.Get(); }
	ID3D12Resource* GetShaderOutputBuffer() { return m_ShaderOutputBuffer.Get(); }
	ID3D12Resource* GetShaderOutputReadbackBuffer() { return m_ShaderOutputReadbackBuffer.Get(); }
	ID3D12Resource* GetShaderOutputUploadBuffer() { return m_ShaderOutputUploadBuffer; }
	ShaderOutput GetShaderOutput();
	UINT QuadricsAmount() { return (UINT)m_Quadrics.size(); }
	void UpdateBuffers(DX12* pDX12);
	Transform& GetTransform() { return m_Transform; }
};