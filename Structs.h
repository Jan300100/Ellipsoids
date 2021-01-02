#pragma once
#include <DirectXMath.h>
#include "Transform.h"
#include "Window.h"
#include <d3d12.h>

struct Quadric
{
	DirectX::XMFLOAT4X4 equation;
	Transform transform;
	DirectX::XMFLOAT3 color = { 1,1,1 };
};

struct InQuadric
{
	DirectX::XMMATRIX transformed;
	DirectX::XMFLOAT3 color;
	InQuadric(const Quadric& src) :transformed{}, color{ src.color }
	{
		DirectX::XMMATRIX tr = DirectX::XMMatrixInverse(nullptr, DirectX::XMMatrixAffineTransformation(DirectX::XMLoadFloat3(&src.transform.scale), DirectX::XMVectorZero()
			, DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(&src.transform.rotation))
			, DirectX::XMLoadFloat3(&src.transform.position)));
		transformed = tr * XMLoadFloat4x4(&src.equation) * XMMatrixTranspose(tr);
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
};

struct AppData
{
	DirectX::XMMATRIX viewProjInv;
	DirectX::XMMATRIX viewInv;
	DirectX::XMMATRIX projInv;
	DirectX::XMUINT4 windowSize;
	DirectX::XMFLOAT4 lightDirection;
	Dimensions<unsigned int> tileDimensions;
	unsigned int quadricsPerRasterizer;
	unsigned int numRasterizers;
};

struct MeshData
{
	DirectX::XMMATRIX transform;
	unsigned int numQuadrics;
};

struct MeshOutputData
{
	unsigned int claimedRasterizers;
	bool overflowed;
};

struct ScreenTile
{
	unsigned int rasterizerHint;
};

struct Rasterizer
{
	unsigned int screenTileIdx; //indicates position on the screen
	unsigned int rasterizerIdx;
	unsigned int nextRasterizerIdx; //if this Tile is saturated with quadrics, they should be added to the next tile instead.
	unsigned int numQuadrics;
};