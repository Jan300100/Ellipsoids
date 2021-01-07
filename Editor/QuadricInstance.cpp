#include "QuadricInstance.h"
#include "QuadricGeometry.h"

using namespace DirectX;

QuadricInstance::QuadricInstance(QuadricGeometry* pGeometry)
    :m_pGeometry{ pGeometry }, m_Transform{}
{
}

DirectX::XMMATRIX QuadricInstance::GetTransformMatrix()
{
    XMMATRIX tr = m_Transform.GetWorld();
    //invert (for second-order surfaces) and transpose (for directX)
    tr = XMMatrixInverse(nullptr, XMMatrixTranspose(tr));
    return tr;
}
