#pragma once
#include <DirectXMath.h>

#include <d3d12.h>
#include <wrl.h>
#include <vector>

using namespace Microsoft::WRL;

struct Quadric
{
	DirectX::XMFLOAT4X4 equation;
	DirectX::XMFLOAT3 position{0,0,0};
	DirectX::XMFLOAT3 rollPitchYaw{0,0,0};
	DirectX::XMFLOAT3 scale{1,1,1};
	DirectX::XMFLOAT3 color;
	DirectX::XMMATRIX Transformed() const;
};

struct OutQuadric
{
	DirectX::XMMATRIX transform;
	DirectX::XMMATRIX normalGenerator;
	DirectX::XMFLOAT3 color;
};