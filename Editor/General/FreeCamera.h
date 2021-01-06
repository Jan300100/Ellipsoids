#pragma once
#include <DirectXMath.h>
#include "Camera.h"
#include "Transform.h"

class Mouse;
class Window;

class FreeCamera : public Camera
{
	Mouse* m_pMouse;
public:
	FreeCamera(Window* pWindow, Mouse* pMouse, const Transform& transform = {});
	virtual void Update(float deltaTime) override;
};