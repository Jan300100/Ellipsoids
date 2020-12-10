#include "Ellipsoid.h"
#include "Camera.h"

using namespace DirectX;

OutEllipsoid::OutEllipsoid(const Ellipsoid& input, Camera* pCamera)
	:color{input.color}
{
	XMMATRIX viewProjInv = pCamera->GetViewProjectionInverse();

	//SHEAR == PER SPHERE
	// to create T_sp -> we need Q_p
	// needs to be calculated every frame --> equivalent of the vertex shader
	XMMATRIX result{ viewProjInv * XMLoadFloat4x4(&input.transform) * XMMatrixTranspose(viewProjInv) };

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
	};

	// now we can create Q_s sheared quadric

	result = (-1 / temp(2, 2)) * (shearMatrix * result * XMMatrixTranspose(shearMatrix));
	XMStoreFloat4x4(&temp, result);

	//now we need to find Q_tilde -> a simplified version of Q_s so its easier to find z
	XMFLOAT3X3 tr
	{
		temp(0,0),temp(0,1),temp(0,3),
		temp(1,0),temp(1,1),temp(1,3),
		temp(3,0),temp(3,1),temp(3,3),
	};

	transform = XMLoadFloat3x3(&tr);
}
