#include "Ellipsoid.h"
#include "Camera.h"

#include <iostream>

using namespace DirectX;

OutEllipsoid::OutEllipsoid(const Ellipsoid& input, Camera* pCamera)
	:color{input.color}
{
	XMMATRIX viewProjInv = pCamera->GetViewProjectionInverse(); //T_pd

	//transformation
	//XMMATRIX surface = XMLoadFloat4x4(&input.equation);
	XMMATRIX surface = input.Transformed();

	//SHEAR == PER SPHERE
	// to create T_sp -> we need Q_p
	// needs to be calculated every frame --> equivalent of the vertex shader
	XMMATRIX result{ viewProjInv * surface * XMMatrixTranspose(viewProjInv) };

	XMFLOAT4X4 temp; XMStoreFloat4x4(&temp, result);
	float shearCol2[4];
	for (size_t i = 0; i < 4; i++)
	{
		shearCol2[i] = -temp(i, 2) / temp(2, 2);
	}

	XMMATRIX shearMatrix
	{
		1,0,shearCol2[0],0,
		0,1,shearCol2[1],0,
		0,0,shearCol2[2],0,
		0,0,shearCol2[3],1
	}; //T_sp

	// now we can create sheared quadric
	result = (-1 / temp(2, 2)) * (shearMatrix * result * XMMatrixTranspose(shearMatrix));
	XMStoreFloat4x4(&temp, result);

	//now we need to find Q_tilde -> a simplified version of the result so its easier to find z
	XMFLOAT3X3 tr
	{
		temp(0,0),temp(0,1),temp(0,3),
		temp(1,0),temp(1,1),temp(1,3),
		temp(3,0),temp(3,1),temp(3,3),
	};

	transform = XMLoadFloat3x3(&tr);
	//XMLoadFloat4x4(&input.equation)
	normalGenerator = (shearMatrix * viewProjInv) * surface * XMMatrixTranspose(pCamera->GetViewInverse());

	XMFLOAT3 pos{ 0,0,1 };
	XMFLOAT3 zSquared;
	XMStoreFloat3(&zSquared, XMVectorMultiply(XMLoadFloat3(&pos), XMVector3Transform(XMLoadFloat3(&pos), transform)));
	if (zSquared.z > 0)
	{
		pos.z = sqrt(zSquared.z);
		XMFLOAT3 normal; XMStoreFloat3(&normal, XMVector3Normalize(XMVector3Transform(XMLoadFloat3(&pos), normalGenerator)));
	}
}

DirectX::XMMATRIX Ellipsoid::Transformed() const
{
	XMMATRIX tr =  XMMatrixAffineTransformation(XMLoadFloat3(&scale), XMVectorZero(), XMQuaternionRotationRollPitchYaw(rollPitchYaw.x,rollPitchYaw.y, rollPitchYaw.z), XMLoadFloat3(&position));
	tr = XMMatrixInverse(nullptr, tr);
	return tr * XMLoadFloat4x4(&equation) * XMMatrixTranspose(tr);
}
