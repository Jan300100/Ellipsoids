#include "Ellipsoid.h"
#include "Camera.h"

#include <iostream>

using namespace DirectX;

DirectX::XMMATRIX Ellipsoid::Transformed() const
{
	XMMATRIX tr =  XMMatrixAffineTransformation(XMLoadFloat3(&scale), XMVectorZero(), XMQuaternionRotationRollPitchYaw(rollPitchYaw.x,rollPitchYaw.y, rollPitchYaw.z), XMLoadFloat3(&position));
	tr = XMMatrixInverse(nullptr, tr);
	return tr * XMLoadFloat4x4(&equation) * XMMatrixTranspose(tr);
}
