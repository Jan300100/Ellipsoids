#pragma once
#include <DirectXMath.h>
#include "Transform.h"
#include <d3d12.h>
#include <wrl.h>
#include <vector>

using namespace Microsoft::WRL;

struct InQuadric
{
	DirectX::XMMATRIX transformed;
	DirectX::XMFLOAT3 color;
};

struct Quadric
{
	DirectX::XMFLOAT4X4 equation;
	Transform transform;
	DirectX::XMFLOAT3 color = {1,1,1};
	InQuadric Transformed() const;
};



struct OutQuadric
{
	DirectX::XMMATRIX shearToProj;
	DirectX::XMMATRIX normalGenerator;
	DirectX::XMMATRIX transform;
	DirectX::XMFLOAT3 color;
	///
	DirectX::XMUINT2 startPixel;
	unsigned int startPatch;
	unsigned int numPatches;
	unsigned int numHorizontalPatches;
};


//struct OutQuadric
//{
//	float4x4 shearToProj;
//	float4x4 covariantTensor;
//	float3x3 transform;
//	float3 color;
//	/////
//	uint2 startPixel;
//	uint startPatch;
//	uint numPatches;
//	uint numHorizontalPatches;
//};