#pragma once
#include "Structs.h"
#include <vector>
#include <wrl.h>
#include <dxgi1_6.h>

class QuadricRenderer;

class QuadricGeometry
{
	friend class QuadricRenderer;

	//data cpu
	std::vector<Quadric> m_Quadrics; 
	std::vector<DirectX::XMMATRIX> m_Transforms; 
	Microsoft::WRL::ComPtr<ID3D12Resource> m_InputBuffer = nullptr; //has the cpu data
	Microsoft::WRL::ComPtr<ID3D12Resource> m_InputUploadBuffer = nullptr; //uploads the inputquadrics data to the gpu
	///PER MESH DATA : //contains the transformation matrix for the entire mesh

	void RecreateMeshBuffer(QuadricRenderer* pRenderer);

	ID3D12Resource* m_MeshDataBuffer = nullptr;
	size_t m_NumInstances;
	bool m_Initialized = false;
	std::string m_Name;
public:
	QuadricGeometry(const std::string& name = "QuadricGeometry");
	~QuadricGeometry();
	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }
	ID3D12Resource* GetInputBuffer() const { return m_InputBuffer.Get(); }
	ID3D12Resource* GetTransformBuffer() { return m_MeshDataBuffer; }
	void UpdateTransforms(ID3D12GraphicsCommandList* pComList, class QuadricRenderer* pRenderer);
	size_t GetNumInstances() const { return m_NumInstances; }
	void Init(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList, const std::vector<Quadric>& quadrics);

	size_t QuadricsAmount() const { return m_Quadrics.size(); }
};