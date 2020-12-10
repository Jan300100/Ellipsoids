#pragma once
#include <DirectXMath.h>
#include "Window.h"
class Mouse;

class Camera
{
	Window* m_pWindow;
	Mouse* m_pMouse;
	float m_Fov = DirectX::XM_PIDIV2;

	float m_NearPlane = 1.0f, m_FarPlane = 10.f;

	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_Forward;

public:
	Camera(Window* pWindow,Mouse* pMouse, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& lookDir);
	DirectX::XMMATRIX GetViewProjectionInverse() const;
	void Offset(const DirectX::XMFLOAT3& offset);
	void Update(float deltaTime);
};
