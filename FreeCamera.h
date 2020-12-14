#pragma once
#include "Camera.h"
#include <DirectXMath.h>
class Mouse;
class Window;

class FreeCamera : public Camera
{
	Mouse* m_pMouse;
public:
	FreeCamera(Window* pWindow, Mouse* pMouse, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& lookDir);
	virtual void Update(float deltaTime) override;
};