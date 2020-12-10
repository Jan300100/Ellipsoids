#pragma once
#include <DirectXMath.h>

#include <d3d12.h>
#include <wrl.h>
#include <vector>

using namespace Microsoft::WRL;

struct Ellipsoid
{
	DirectX::XMFLOAT4X4 transform;
	DirectX::XMFLOAT3 color;
};

struct OutEllipsoid
{
	OutEllipsoid(const Ellipsoid& input, class Camera* pCamera);
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