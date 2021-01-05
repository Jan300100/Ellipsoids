#pragma once
#include <DirectXMath.h>

struct Transform
{
	DirectX::XMFLOAT3 position, rotation, scale{1,1,1};
	DirectX::XMMATRIX matrix;
	void CalculateMatrix();
};