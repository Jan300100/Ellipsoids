#include "FreeCamera.h"
#include "Mouse.h"

using namespace DirectX;


FreeCamera::FreeCamera(Window* pWindow, Mouse* pMouse, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& lookDir)
	:Camera{ pWindow, pos, lookDir }, m_pMouse{ pMouse }{}

void FreeCamera::Update(float deltaTime)
{
	XMFLOAT3 translation{};
	XMFLOAT3 rotation{};
	float rotationSpeed = 4.0f;
	float moveSpeed = 4.0f;
	//if (m_pMouse->GetMouseButton(Mouse::Button::Right) == ButtonState::Down
	//	&& m_pMouse->GetMouseButton(Mouse::Button::Left) == ButtonState::Down)
	//{
	//	XMVECTOR forward{ XMLoadFloat3(&m_Forward) };
	//	XMVECTOR right = XMVector3Cross(forward, XMVectorSet(0, 1, 0, 0));
	//	right = XMVectorScale(right, -m_pMouse->GetMouseDelta().x * moveSpeed * deltaTime);
	//	XMStoreFloat3(&translation, right);
	//	translation.y += -(float)(m_pMouse->GetMouseDelta().y) * moveSpeed * deltaTime;
	//}
	if (m_pMouse->GetMouseButton(Mouse::Button::Left) == ButtonState::Down)
	{
		XMVECTOR forward{ XMLoadFloat3(&m_Forward) };
		forward = XMVector3Normalize(forward);
		forward = XMVectorScale(forward, -m_pMouse->GetMouseDelta().y * moveSpeed * deltaTime);
		XMFLOAT3 f; XMStoreFloat3(&f, forward);
		translation = f;
		rotation.y = (float)(m_pMouse->GetMouseDelta().x) * rotationSpeed * deltaTime;
	}
	else if (m_pMouse->GetMouseButton(Mouse::Button::Right) == ButtonState::Down)
	{
		rotation = { (float)(m_pMouse->GetMouseDelta().y) * rotationSpeed * deltaTime, (float)(m_pMouse->GetMouseDelta().x) * rotationSpeed * deltaTime, 0 };
	}

	//TRANSLATION
	Offset(translation);

	//ROTATION
	XMVECTOR quat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));
	XMVECTOR forward{ XMLoadFloat3(&m_Forward) };
	forward = XMVector3Rotate(forward, quat);
	XMStoreFloat3(&m_Forward, forward);

	ReCalculateView();
}
