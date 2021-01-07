#pragma once
#include "QuadricInstance.h"
#include "Transform.h"
#include <string>
#include <vector>
#include <dxgi1_6.h>

class QuadricRenderer;
class QuadricGeometry;

class SceneNode
{
	std::string m_Name;
	Transform m_Transform;
	std::vector<QuadricInstance*> m_pElements;
	std::vector<SceneNode*> m_pChildNodes;
public:
	~SceneNode();
	SceneNode(const std::string& name = "SceneNode");
	SceneNode(const SceneNode& other);
	SceneNode(SceneNode&&) = delete;
	SceneNode& operator=(const SceneNode& other);
	SceneNode& operator=(SceneNode&&) = delete;

	void AddElement(QuadricInstance* pElement);
	void RemoveElement(QuadricInstance* pElement, bool destroy = true);

	void Clear();

	void AddNode(SceneNode* pNode);
	void RemoveNode(SceneNode* pNode, bool destroy = true);

	std::string GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }
	void RenderImGui(SceneNode** outSelectedNode, QuadricInstance** outSelectedInstance);
	void RenderEditImGui();
	SceneNode* GetParent();
	void Render(QuadricRenderer* pQRenderer) const;
};