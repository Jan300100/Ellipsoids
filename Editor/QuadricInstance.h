#pragma once
#include <wtypes.h>
#include "Transform.h"
#include <DirectXMath.h>

class QuadricGeometry;

class QuadricInstance
{
	QuadricGeometry* m_pGeometry;
	Transform m_Transform;
public:
	QuadricInstance(QuadricGeometry* pGeometry);
	Transform GetTransform() const { return m_Transform; }
	DirectX::XMMATRIX GetTransformMatrix();
	void SetTransform(const Transform& tr);
	QuadricGeometry* GetGeometry() const { return m_pGeometry; }
};