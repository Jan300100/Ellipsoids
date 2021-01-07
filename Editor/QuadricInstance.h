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
	Transform& GetTransform() { return m_Transform; }
	DirectX::XMMATRIX GetTransformMatrix();
	QuadricGeometry* GetGeometry() const { return m_pGeometry; }
};