#include "Camera.h"
#include "Window.h"
#include <iostream>
#include <Windowsx.h>
#include "Mouse.h"

using namespace DirectX;

void Camera::ReCalculateView()
{
	XMVECTOR forward{ GetForward() };
	XMVECTOR right{ GetRight() };
	XMVECTOR up = XMVector3Cross(forward , right);
	XMFLOAT3 posf3 = m_Transform.GetPosition();
	XMVECTOR pos = XMLoadFloat3(&posf3);
	XMVECTOR target = XMVectorAdd(pos, forward);
	m_View = XMMatrixLookAtLH(pos, target, up);

	m_ViewInverse = XMMatrixInverse(nullptr, m_View);
	m_ViewProj = m_View * m_Projection;
	m_ViewProjInverse =  XMMatrixInverse(nullptr, m_ViewProj);
}

void Camera::CalculateProj()
{
	m_Projection = XMMatrixPerspectiveFovLH(m_Fov, float(m_pWindow->GetDimensions().width) / m_pWindow->GetDimensions().height, m_NearPlane, m_FarPlane);
}

Camera::Camera(Window* pWindow, const Transform& transform)
	:m_pWindow{ pWindow }, m_Transform{ transform }
{
	CalculateProj();
}

DirectX::XMVECTOR Camera::GetForward() const
{
	XMFLOAT3 rot = m_Transform.GetRotation();
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rot));
	XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotationMatrix);
	return XMVector3Normalize(forward);
}

DirectX::XMVECTOR Camera::GetRight() const
{
	XMFLOAT3 rot = m_Transform.GetRotation();
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rot));
	XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotationMatrix);
	return XMVector3Normalize(right);
}

void Camera::Offset(const DirectX::XMFLOAT3& offset)
{
	XMFLOAT3 pos = m_Transform.GetPosition();
	XMStoreFloat3(&pos, XMVectorAdd(XMLoadFloat3(&pos), XMLoadFloat3(&offset)));
	m_Transform.SetPosition(pos);
}