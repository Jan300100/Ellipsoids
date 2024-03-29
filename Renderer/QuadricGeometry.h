#pragma once
#include "GPUBuffer.h"
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
	GPUBuffer m_InputBuffer;
	void RecreateInstanceBuffer(QuadricRenderer* pRenderer);

	GPUBuffer m_InstanceBuffer;

	GPUBuffer m_DrawDataBuffer; //contains srv indices

	size_t m_NumInstances;
	bool m_Initialized = false;
	std::string m_Name;
public:
	QuadricGeometry(const std::string& name = "QuadricGeometry");
	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }
	ID3D12Resource* GetDrawData() const { return m_DrawDataBuffer.Get(); }
	void UpdateTransforms(ID3D12GraphicsCommandList* pComList, class QuadricRenderer* pRenderer);
	size_t GetNumInstances() const { return m_NumInstances; }
	void Init(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList, const std::vector<Quadric>& quadrics);

	size_t QuadricsAmount() const { return m_Quadrics.size(); }
};