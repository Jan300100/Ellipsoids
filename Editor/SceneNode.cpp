#include "SceneNode.h"
#include "QuadricRenderer.h"
#include "imgui.h"
#include "QuadricGeometry.h"

SceneNode::~SceneNode()
{
	for (QuadricInstance* pInst : m_pElements)
	{
		delete pInst;
	}
	for (SceneNode* pNode : m_pChildNodes)
	{
		delete pNode;
	}
}

void SceneNode::AddElement(QuadricInstance* pElement)
{
	m_pElements.push_back(pElement);
	pElement->GetTransform().SetParent(&m_Transform);
}

void SceneNode::RemoveElement(QuadricInstance* pElement)
{
	pElement->GetTransform().SetParent(nullptr);
	for (size_t i = 0; i < m_pElements.size(); i++)
	{
		if (m_pElements[i] == pElement)
		{
			m_pElements[i] = m_pElements.back();
			m_pElements.pop_back();
			return;
		}
	}
}

void SceneNode::AddNode(SceneNode* pNode)
{
	m_pChildNodes.push_back(pNode);
	pNode->m_Transform.SetParent(&m_Transform);
}

void SceneNode::RemoveNode(SceneNode* pNode)
{
	pNode->m_Transform.SetParent(nullptr);
	for (size_t i = 0; i < m_pChildNodes.size(); i++)
	{
		if (m_pChildNodes[i] == pNode)
		{
			m_pChildNodes[i] = m_pChildNodes.back();
			m_pChildNodes.pop_back();
			return;
		}
	}
}

void SceneNode::RenderImGui()
{
	if (ImGui::TreeNode("SceneNode"))
	{
		for (SceneNode* pNode : m_pChildNodes)
		{
			pNode->RenderImGui();
		}
		for (QuadricInstance* pInst : m_pElements)
		{
			ImGui::Text((pInst->GetGeometry()->GetName() + "::Instance").c_str());
		}
		ImGui::TreePop();
	}
}

void SceneNode::Render(QuadricRenderer* pQRenderer) const
{
	for (QuadricInstance* pInst : m_pElements)
	{
		pQRenderer->Render(pInst->GetGeometry(), pInst->GetTransformMatrix());
	}
	for (SceneNode* pNode : m_pChildNodes)
	{
		pNode->Render(pQRenderer);
	}
}
