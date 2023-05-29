#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include "Helpers.h"

struct Quadric
{
	DirectX::XMMATRIX transformed;
	DirectX::XMFLOAT3 color;
	Quadric() = default;
	Quadric(const DirectX::XMFLOAT4X4& equation, const DirectX::XMMATRIX& transform, const DirectX::XMFLOAT3& color) :transformed{}, color{ color }
	{
		DirectX::XMMATRIX tr = transform;
		tr = XMMatrixInverse(nullptr, tr);
		transformed = tr * XMLoadFloat4x4(&equation) * XMMatrixTranspose(tr);
	}
};

struct OutQuadric
{
	DirectX::XMMATRIX shearToProj;
	DirectX::XMMATRIX normalGenerator;
	DirectX::XMMATRIX transform;
	DirectX::XMFLOAT3 color;
	DirectX::XMFLOAT2 yRange;
	DirectX::XMFLOAT2 xRange;

	DirectX::XMUINT2 bbStart;
	DirectX::XMUINT2 bbEnd;
};

struct AppData
{
	DirectX::XMMATRIX viewProjInv;
	DirectX::XMMATRIX viewInv;
	DirectX::XMMATRIX projInv;
	DirectX::XMUINT4 windowSize;
	DirectX::XMFLOAT4 lightDirection;

	Dimensions<unsigned int> tileDimensions;
	unsigned int batchSize;

	unsigned int depthUAVIdx;
	unsigned int colorUAVIdx;
};

