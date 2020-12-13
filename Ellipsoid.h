#pragma once
#include <DirectXMath.h>

#include <d3d12.h>
#include <wrl.h>
#include <vector>

using namespace Microsoft::WRL;

struct Ellipsoid
{
	DirectX::XMFLOAT4X4 equation;
	DirectX::XMFLOAT3 position{0,0,0};
	DirectX::XMFLOAT3 rollPitchYaw{0,0,0};
	DirectX::XMFLOAT3 scale{1,1,1};
	DirectX::XMFLOAT3 color;
	DirectX::XMMATRIX Transformed() const;
};

struct OutEllipsoid
{
	DirectX::XMMATRIX transform;
	DirectX::XMMATRIX normalGenerator;
	DirectX::XMFLOAT3 color;
};

class EllipsoidMesh
{
	std::vector<Ellipsoid> m_Ellipsoids; //data cpu
	ComPtr<ID3D12Resource> m_InputBuffer; //original ellipsoids
	ComPtr<ID3D12Resource> m_OutputBuffer; //output after projection
};