#pragma once
#include <DirectXMath.h>
#include "Window.h"

class Camera
{
	//MATRICES
	DirectX::XMMATRIX m_View;
	DirectX::XMMATRIX m_ViewInverse;
	DirectX::XMMATRIX m_ViewProj;
	DirectX::XMMATRIX m_ViewProjInverse;
	DirectX::XMMATRIX m_Projection;
protected:
	void ReCalculateView();
	void CalculateProj();
	Window* m_pWindow;
	float m_Fov = DirectX::XM_PIDIV2;
	float m_NearPlane = 1.0f, m_FarPlane = 15.f;

	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_Forward;
public:
	Camera(Window* pWindow, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& lookDir);
	virtual ~Camera() = default;
	inline DirectX::XMMATRIX GetViewProjectionInverse() const { return m_ViewProjInverse; }
	inline DirectX::XMMATRIX GetViewProjection() const { return m_ViewProj; }
	inline DirectX::XMMATRIX GetViewInverse() const { return m_ViewInverse; }
	inline DirectX::XMMATRIX GetView() const { return m_View; }
	void Offset(const DirectX::XMFLOAT3& offset);
	virtual void Update(float) {};
};
