#include "Quadric.h"
#include "Camera.h"

#include <iostream>

using namespace DirectX;

InQuadric Quadric::Transformed() const
{
	InQuadric out;
	XMMATRIX tr =  XMMatrixAffineTransformation(XMLoadFloat3(&transform.scale), XMVectorZero()
		, XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&transform.rotation))
		, XMLoadFloat3(&transform.position));
	tr = XMMatrixInverse(nullptr, tr);
	out.transformed = tr * XMLoadFloat4x4(&equation) * XMMatrixTranspose(tr);
	out.color = color;
	return out;
}
