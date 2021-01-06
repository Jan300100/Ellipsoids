#pragma once
#include <DirectXMath.h>


class Transform final
{
	DirectX::XMFLOAT3 m_Position, m_Rotation, m_Scale{ 1,1,1 };
	DirectX::XMMATRIX m_Matrix;
	void CalculateMatrix();
	Transform* m_pParent = nullptr;
	bool m_Dirty = true;
public:
	Transform() = default;
	Transform(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot, const DirectX::XMFLOAT3& scale, Transform* pParent = nullptr);
	void SetParent(Transform* pParent) { m_pParent = pParent; }
	void SetPosition(const DirectX::XMFLOAT3& pos) { m_Dirty = true; m_Position = pos; }
	void SetRotation(const DirectX::XMFLOAT3& rot) { m_Dirty = true; m_Rotation = rot; }
	void SetScale(const DirectX::XMFLOAT3& scale) { m_Dirty = true; m_Scale = scale; }

	DirectX::XMFLOAT3 GetPosition() const { return m_Position; }
	DirectX::XMFLOAT3 GetRotation() const { return m_Rotation; }
	DirectX::XMFLOAT3 GetScale()const { return m_Scale; }

	DirectX::XMMATRIX GetWorld();
};