#include "Transform.h"

using namespace DirectX;

void Transform::CalculateMatrix()
{
    m_Matrix = XMMatrixAffineTransformation(XMLoadFloat3(&m_Scale), XMVectorZero()
        , XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_Rotation))
        , XMLoadFloat3(&m_Position));
    m_Dirty = false;
}

Transform::Transform(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot, const DirectX::XMFLOAT3& scale, Transform* pParent)
    :m_Position{ pos }, m_Rotation{ rot }, m_Scale{ scale }, m_Dirty{ true }, m_pParent{ pParent }, m_Matrix{}
{
}

DirectX::XMMATRIX Transform::GetWorld()
{
    if (m_Dirty) CalculateMatrix();

    XMMATRIX world = m_Matrix;
    if (m_pParent)
    {
        world *= m_pParent->GetWorld();
    }
    return world;
}
