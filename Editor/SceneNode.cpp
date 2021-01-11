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

SceneNode::SceneNode(const std::string& name)
	:m_Name{ name }, m_Transform{}, m_pChildNodes{}, m_pElements{}
{
}

SceneNode::SceneNode(const SceneNode& other)
	: m_Name{other.m_Name}, m_Transform {
	other.m_Transform
}, m_pElements{}, m_pChildNodes{}
{
	for (QuadricInstance* pInst : other.m_pElements)
	{
		AddElement(new QuadricInstance{ (*pInst) });
	}
	for (SceneNode* pNode : other.m_pChildNodes)
	{
		AddNode(new SceneNode{ (*pNode) });
	}
}

SceneNode& SceneNode::operator=(const SceneNode& other)
{
	m_Transform = other.m_Transform;
	m_Name = other.m_Name;
	for (QuadricInstance* pInst : other.m_pElements)
	{
		AddElement(new QuadricInstance{ (*pInst) });
	}
	for (SceneNode* pNode : other.m_pChildNodes)
	{
		AddNode(new SceneNode{ (*pNode) });
	}

	return (*this);
}

void SceneNode::AddElement(QuadricInstance* pElement)
{
	m_pElements.push_back(pElement);
	pElement->GetTransform().SetParent(&m_Transform, this);
}

void SceneNode::RemoveElement(QuadricInstance* pElement, bool destroy)
{
	pElement->GetTransform().SetParent(nullptr);
	for (size_t i = 0; i < m_pElements.size(); i++)
	{
		if (m_pElements[i] == pElement)
		{
			m_pElements[i] = m_pElements.back();
			m_pElements.pop_back();
			if (destroy)
				delete pElement;
			return;
		}
	}
}

void SceneNode::Clear()
{
	std::vector<QuadricInstance*> elements{ m_pElements };
	for (QuadricInstance* pE : elements)
	{
		RemoveElement(pE);
	}

	std::vector<SceneNode*> nodes{ m_pChildNodes };
	for (SceneNode* pN : nodes)
	{
		RemoveNode(pN);
	}
}

void SceneNode::AddNode(SceneNode* pNode)
{
	m_pChildNodes.push_back(pNode);
	pNode->m_Transform.SetParent(&m_Transform, this);
}

void SceneNode::RemoveNode(SceneNode* pNode, bool destroy)
{
	pNode->m_Transform.SetParent(nullptr);
	for (size_t i = 0; i < m_pChildNodes.size(); i++)
	{
		if (m_pChildNodes[i] == pNode)
		{
			m_pChildNodes[i] = m_pChildNodes.back();
			m_pChildNodes.pop_back();
			if (destroy)
				delete pNode;
			return;
		}
	}
}

void SceneNode::RenderImGui(SceneNode** outSelectedNode, QuadricInstance** outSelectedInstance)
{
	ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

	if (ImGui::TreeNodeEx(this, node_flags, m_Name.c_str()))
	{
		if (ImGui::IsItemClicked())
		{
			(*outSelectedNode) = this;
		}
		for (SceneNode* pNode : m_pChildNodes)
		{
			pNode->RenderImGui(outSelectedNode , outSelectedInstance);
		}
		for (QuadricInstance* pInst : m_pElements)
		{
			node_flags |= ImGuiTreeNodeFlags_Leaf; 
			if (ImGui::TreeNodeEx(pInst, node_flags, (pInst->GetGeometry()->GetName() + "::Instance").c_str()))
			{
				if (ImGui::IsItemClicked())
				{
					(*outSelectedInstance) = pInst;
				}
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}
}

void SceneNode::RenderEditImGui()
{

	char buf[64]{};
	strcpy_s(buf, m_Name.c_str());
	if (ImGui::InputText("Name", buf, 64))
	{
		m_Name = buf;
	}
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

SceneNode* SceneNode::GetParent()
{
	return (SceneNode*)(m_Transform.GetParentData());
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
