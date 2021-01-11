#include "FreeCamera.h"
#include "Mouse.h"
#include <iostream>

using namespace DirectX;

FreeCamera::FreeCamera(Window* pWindow, Mouse* pMouse, const Transform& transform)
	:Camera{ pWindow, transform }, m_pMouse{ pMouse }{}

void FreeCamera::Update(float)
{
	XMVECTOR forward{ GetForward() };
	XMFLOAT3 translation{};
	XMFLOAT3 rotation{};
	float sensitivity = 0.005f;


	if (m_pMouse->GetMouseButton(Mouse::Button::Right) == ButtonState::Down
		&& m_pMouse->GetMouseButton(Mouse::Button::Left) == ButtonState::Down)
	{
		XMVECTOR right = GetRight();
		right = XMVectorScale(right, m_pMouse->GetMouseDelta().x * sensitivity);
		XMStoreFloat3(&translation, right);
		translation.y += (float)(-m_pMouse->GetMouseDelta().y) * sensitivity;
	}
	else if (m_pMouse->GetMouseButton(Mouse::Button::Left) == ButtonState::Down)
	{
		forward = XMVectorScale(forward, -m_pMouse->GetMouseDelta().y * sensitivity);
		XMFLOAT3 f; XMStoreFloat3(&f, forward);
		translation = f;
		rotation.y = (float)(m_pMouse->GetMouseDelta().x) * sensitivity;
	}
	else if (m_pMouse->GetMouseButton(Mouse::Button::Right) == ButtonState::Down)
	{
		rotation = { (float)(m_pMouse->GetMouseDelta().y) * sensitivity, (float)(m_pMouse->GetMouseDelta().x) * sensitivity, 0 };
	}

	//TRANSLATION
	XMFLOAT3 pos = m_Transform.GetPosition();
	XMStoreFloat3(&pos, XMVectorAdd(XMLoadFloat3(&pos), XMLoadFloat3(&translation)));
	m_Transform.SetPosition(pos);
	//ROTATION
	XMFLOAT3 rot = m_Transform.GetRotation();
	XMStoreFloat3(&rot, XMVectorAdd(XMLoadFloat3(&rot), XMLoadFloat3(&rotation)));
	m_Transform.SetRotation(rot);

	ReCalculateView();
}
