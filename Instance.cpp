#include "Instance.h"
#include "QuadricGeometry.h"

using namespace DirectX;

Instance::Instance(QuadricGeometry* pGeometry)
    :m_pGeometry{ pGeometry }, m_Transform{}
{
    SetTransform(m_Transform);
}

DirectX::XMMATRIX Instance::GetTransformMatrix()
{
    XMMATRIX tr = m_Transform.GetWorld();
    //invert (for second-order surfaces) and transpose (for directX)
    tr = XMMatrixInverse(nullptr, XMMatrixTranspose(tr));
    return tr;
}

void Instance::SetTransform(const Transform& tr)
{
    m_Transform = tr;
}
