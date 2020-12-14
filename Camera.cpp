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

Camera::Camera(Window* pWindow, const XMFLOAT3& pos, const XMFLOAT3& lookDir)
	:m_pWindow{ pWindow }, m_Position{ pos }, m_Forward{ lookDir }
{
	CalculateProj();
}

void Camera::Offset(const DirectX::XMFLOAT3& offset)
{
	m_Position.x += offset.x;
	m_Position.y += offset.y;
	m_Position.z += offset.z;
}