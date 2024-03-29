#pragma once
#include <DirectXMath.h>
#include "Transform.h"
class Window;
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
	float m_Fov = 1.0f;
	float m_NearPlane = 1.0f, m_FarPlane = 100.0f;

	Transform m_Transform;
public:
	Camera(Window* pWindow, const Transform& transform);
	virtual ~Camera() = default;
	inline DirectX::XMMATRIX GetViewProjectionInverse() const { return m_ViewProjInverse; }
	inline DirectX::XMMATRIX GetViewProjection() const { return m_ViewProj; }
	inline DirectX::XMMATRIX GetViewInverse() const { return m_ViewInverse; }
	inline DirectX::XMMATRIX GetView() const { return m_View; }
	float GetNearPlane() const { return m_NearPlane; }
	float GetFarPlane() const { return m_FarPlane; }
	float GetFOV() const { return m_Fov; }
	DirectX::XMVECTOR GetForward() const;
	DirectX::XMVECTOR GetRight() const;
	void Offset(const DirectX::XMFLOAT3& offset);
	virtual void Update(float) {};
};
