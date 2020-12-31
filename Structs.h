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
	unsigned int quadricsPerTile;
	float multiplier;
};

struct MeshData
{
	DirectX::XMMATRIX transform;
	unsigned int numQuadrics;
};

struct ScreenTile
{
	unsigned int tileIndex;
	unsigned int numQuadrics;
};

struct Tile
{
	unsigned int screenTileIndex; //indicates position on the screen
	unsigned int quadricStartIndex;
	unsigned int quadricsReserved; //set by screentile
	unsigned int quadricCtr; //used by quadrics that want to add themselves to this buffer
	unsigned int nextTileIndex; //if this Tile is saturated with quadrics, they should be added to the next tile instead.
};