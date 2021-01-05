#include "Instance.h"
#include "QuadricGeometry.h"

using namespace DirectX;

Instance::Instance(QuadricGeometry* pGeometry)
    :m_pGeometry{ pGeometry }, m_Transform{}
{
    SetTransform(m_Transform);
}

void Instance::SetTransform(const Transform& tr)
{
    m_Transform = tr;
    m_Transform.CalculateMatrix();
}
