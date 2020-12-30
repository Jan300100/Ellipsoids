#include "Quadric.h"
#include "Camera.h"

#include <iostream>

using namespace DirectX;

InQuadric::InQuadric(const Quadric& src)
	:transformed{}, color{src.color}
{
	XMMATRIX tr = XMMatrixInverse(nullptr, XMMatrixAffineTransformation(XMLoadFloat3(&src.transform.scale), XMVectorZero()
		, XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&src.transform.rotation))
		, XMLoadFloat3(&src.transform.position)));
	transformed = tr * XMLoadFloat4x4(&src.equation) * XMMatrixTranspose(tr);
}
