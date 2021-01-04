#pragma once
#include <wtypes.h>
#include "Transform.h"
#include <DirectXMath.h>

class QuadricMesh;

class Instance
{
	QuadricMesh* m_pMesh;
	Transform m_Transform;
	DirectX::XMMATRIX m_TransformationMatrix;
public:
	Instance(QuadricMesh* pMesh);
	void Render() const;
	Transform GetTransform() { return m_Transform; }
	void SetTransform(const Transform& tr);
};