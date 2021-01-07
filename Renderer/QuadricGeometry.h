#pragma once
#include "Structs.h"
#include <vector>
#include <wrl.h>
#include <dxgi1_6.h>


class QuadricGeometry
{
	friend class QuadricRenderer;
	//data cpu
	UINT m_MaxInstances;
	std::vector<Quadric> m_Quadrics; 
	std::vector<DirectX::XMMATRIX> m_Transforms; 
	Microsoft::WRL::ComPtr<ID3D12Resource> m_InputBuffer = nullptr; //has the cpu data
	Microsoft::WRL::ComPtr<ID3D12Resource> m_InputUploadBuffer = nullptr; //uploads the inputquadrics data to the gpu
	///PER MESH DATA : //contains the transformation matrix for the entire mesh
	Microsoft::WRL::ComPtr<ID3D12Resource> m_MeshDataBuffer = nullptr;
	bool m_Initialized = false;
	std::string m_Name;
public:
	QuadricGeometry(UINT maxInstances, const std::string& name = "QuadricGeometry");
	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }
	ID3D12Resource* GetInputBuffer() const { return m_InputBuffer.Get(); }
	ID3D12Resource* GetTransformBuffer() { return m_MeshDataBuffer.Get(); }
	UINT UpdateTransforms();

	UINT GetMaxInstances() const { return m_MaxInstances; }
	void Init(ID3D12Device2* pDevice, ID3D12GraphicsCommandList* pComList, const std::vector<Quadric>& quadrics ,UINT maxInstances = 1);

	UINT QuadricsAmount() const { return (UINT)m_Quadrics.size(); }
};