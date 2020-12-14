#include "FreeCamera.h"
#include "Mouse.h"
#include <iostream>

using namespace DirectX;

FreeCamera::FreeCamera(Window* pWindow, Mouse* pMouse, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& lookDir)
	:Camera{ pWindow, pos, lookDir }, m_pMouse{ pMouse }{}

void FreeCamera::Update(float deltaTime)
{
	XMVECTOR forward{ GetForward() };
	XMFLOAT3 translation{};
	XMFLOAT3 rotation{};
	float sensitivity = 3.0f;


	if (m_pMouse->GetMouseButton(Mouse::Button::Right) == ButtonState::Down
		&& m_pMouse->GetMouseButton(Mouse::Button::Left) == ButtonState::Down)
	{
		XMVECTOR right = GetRight();
		right = XMVectorScale(right, m_pMouse->GetMouseDelta().x * sensitivity * deltaTime);
		XMStoreFloat3(&translation, right);
		translation.y += (float)(-m_pMouse->GetMouseDelta().y) * sensitivity * deltaTime;
	}
	else if (m_pMouse->GetMouseButton(Mouse::Button::Left) == ButtonState::Down)
	{
		forward = XMVectorScale(forward, -m_pMouse->GetMouseDelta().y * sensitivity * deltaTime);
		XMFLOAT3 f; XMStoreFloat3(&f, forward);
		translation = f;
		rotation.y = (float)(m_pMouse->GetMouseDelta().x) * sensitivity * deltaTime;
	}
	else if (m_pMouse->GetMouseButton(Mouse::Button::Right) == ButtonState::Down)
	{
		rotation = { (float)(m_pMouse->GetMouseDelta().y) * sensitivity * deltaTime, (float)(m_pMouse->GetMouseDelta().x) * sensitivity * deltaTime, 0 };
	}

	//TRANSLATION
	XMStoreFloat3(&m_Position, XMVectorAdd(XMLoadFloat3(&m_Position), XMLoadFloat3(&translation)));

	//ROTATION
	XMStoreFloat3(&m_Rotation, XMVectorAdd(XMLoadFloat3(&m_Rotation), XMLoadFloat3(&rotation)));

	ReCalculateView();

	

}
