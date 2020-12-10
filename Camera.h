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

	//MATRICES
	DirectX::XMMATRIX m_View;
	DirectX::XMMATRIX m_ViewInverse;
	DirectX::XMMATRIX m_ViewProjInverse;
	DirectX::XMMATRIX m_Projection;

	void ReCalculateView();
	void CalculateProj();
public:
	Camera(Window* pWindow,Mouse* pMouse, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& lookDir);
	inline DirectX::XMMATRIX GetViewProjectionInverse() const { return m_ViewProjInverse; }
	inline DirectX::XMMATRIX GetViewInverse() const { return m_ViewInverse; }
	inline DirectX::XMMATRIX GetView() const { return m_View; }
	void Offset(const DirectX::XMFLOAT3& offset);
	void Update(float deltaTime);
};
