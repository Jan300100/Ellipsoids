#pragma once
#include <DirectXMath.h>
#include "Transform.h"
#include <d3d12.h>
#include <wrl.h>
#include <vector>

using namespace Microsoft::WRL;


struct Quadric
{
	DirectX::XMFLOAT4X4 equation;
	Transform transform;
	DirectX::XMFLOAT3 color = {1,1,1};
};

struct InQuadric
{
	DirectX::XMMATRIX transformed;
	DirectX::XMFLOAT3 color;
	InQuadric(const Quadric& src);
};

struct OutQuadric
{
	DirectX::XMMATRIX shearToProj;
	DirectX::XMMATRIX normalGenerator;
	DirectX::XMMATRIX transform;
	DirectX::XMFLOAT3 color;
	DirectX::XMFLOAT2 yRange;
	DirectX::XMFLOAT2 xRange;
};