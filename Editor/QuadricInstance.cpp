#include "QuadricInstance.h"
#include "QuadricGeometry.h"
#include "imgui.h"
#include "SceneNode.h"

using namespace DirectX;

QuadricInstance::QuadricInstance(QuadricGeometry* pGeometry)
    :m_pGeometry{ pGeometry }, m_Transform{}
{
}

DirectX::XMMATRIX QuadricInstance::GetTransformMatrix()
{
    return  m_Transform.GetWorld();
}

void QuadricInstance::RenderEditImGui()
{

    DirectX::XMFLOAT3 pos = m_Transform.GetPosition();
    DirectX::XMFLOAT3 rot = m_Transform.GetRotation();
    DirectX::XMFLOAT3 sc = m_Transform.GetScale();

    ImGui::DragFloat3("Position", (float*)&pos, 0.1f);
    ImGui::DragFloat3("Rotation", (float*)&rot, 0.1f);
    ImGui::DragFloat3("Scale", (float*)&sc, 0.1f);

    m_Transform.SetPosition(pos);
    m_Transform.SetRotation(rot);
    m_Transform.SetScale(sc);

    ImGui::NewLine();

}

SceneNode* QuadricInstance::GetParent()
{
    return (SceneNode*)m_Transform.GetParentData();
}
