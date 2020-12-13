#include "Camera.h"
#include "Window.h"
#include <iostream>
#include <Windowsx.h>
#include "Mouse.h"

using namespace DirectX;

void Camera::ReCalculateView()
{

	m_View = XMMatrixLookAtLH(XMLoadFloat3(&m_Position), XMVectorAdd(XMLoadFloat3(&m_Position), XMLoadFloat3(&m_Forward)), XMVectorSet(0, 1, 0, 0));
	m_ViewInverse = XMMatrixInverse(nullptr, m_View);
	m_ViewProj = m_View * m_Projection;
	m_ViewProjInverse =  XMMatrixInverse(nullptr, m_ViewProj);

}

void Camera::CalculateProj()
{
	m_Projection = XMMatrixPerspectiveFovLH(m_Fov, float(m_pWindow->GetDimensions().width) / m_pWindow->GetDimensions().height, m_NearPlane, m_FarPlane);
}

Camera::Camera(Window* pWindow, Mouse* pMouse, const XMFLOAT3& pos, const XMFLOAT3& lookDir)
	:m_pWindow{ pWindow }, m_pMouse{pMouse}, m_Position{ pos }, m_Forward{ lookDir }
{
	CalculateProj();
}

void Camera::Offset(const DirectX::XMFLOAT3& offset)
{
	m_Position.x += offset.x;
	m_Position.y += offset.y;
	m_Position.z += offset.z;
}

void Camera::Update(float dt)
{

	XMFLOAT3 translation{};
	XMFLOAT3 rotation{};
	float rotationSpeed = 4.0f;
	float moveSpeed = 4.0f;
	if (m_pMouse->GetMouseButton(Mouse::Button::Right) == ButtonState::Down
		&& m_pMouse->GetMouseButton(Mouse::Button::Left) == ButtonState::Down)
	{
		XMVECTOR forward{ XMLoadFloat3(&m_Forward) };
		XMVECTOR right = XMVector3Cross(forward, XMVectorSet(0, 1, 0,0));
		right = XMVectorScale(right, -m_pMouse->GetMouseDelta().x * moveSpeed * dt);
		XMStoreFloat3(&translation, right);
		translation.y += -(float)(m_pMouse->GetMouseDelta().y) * moveSpeed * dt;
	}
	else if (m_pMouse->GetMouseButton(Mouse::Button::Left) == ButtonState::Down)
	{
		XMVECTOR forward{ XMLoadFloat3(&m_Forward) };
		forward = XMVector3Normalize(forward);
		forward = XMVectorScale(forward, -m_pMouse->GetMouseDelta().y * moveSpeed * dt);
		XMFLOAT3 f; XMStoreFloat3(&f, forward);
		translation = f;
		rotation.y = (float)(m_pMouse->GetMouseDelta().x) * rotationSpeed * dt;
	}
	else if (m_pMouse->GetMouseButton(Mouse::Button::Right) == ButtonState::Down)
	{
		rotation = { (float)(m_pMouse->GetMouseDelta().y) * rotationSpeed * dt, (float)(m_pMouse->GetMouseDelta().x) * rotationSpeed * dt, 0 };
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
