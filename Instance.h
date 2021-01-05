#pragma once
#include <wtypes.h>
#include "Transform.h"
#include <DirectXMath.h>

class QuadricGeometry;

class Instance
{
	QuadricGeometry* m_pGeometry;
	Transform m_Transform;
public:
	Instance(QuadricGeometry* pGeometry);
	Transform GetTransform() const { return m_Transform; }
	const DirectX::XMMATRIX& GetTransformMatrix() const { return m_Transform.matrix; }
	void SetTransform(const Transform& tr);
	QuadricGeometry* GetGeometry() const { return m_pGeometry; }
};