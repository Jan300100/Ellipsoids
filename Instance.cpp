#include "Instance.h"
#include "QuadricMesh.h"

using namespace DirectX;

Instance::Instance(QuadricMesh* pMesh)
    :m_pMesh{ pMesh }, m_Transform{}
{
    SetTransform(m_Transform);
}

void Instance::Render() const
{
    m_pMesh->m_Transforms.push_back(m_TransformationMatrix);
}

void Instance::SetTransform(const Transform& tr)
{
    m_Transform = tr;
    m_TransformationMatrix = XMMatrixAffineTransformation(XMLoadFloat3(&m_Transform.scale), XMVectorZero()
        , XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_Transform.rotation))
        , XMLoadFloat3(&m_Transform.position));
    m_TransformationMatrix = XMMatrixInverse(nullptr, XMMatrixTranspose(m_TransformationMatrix));
}
