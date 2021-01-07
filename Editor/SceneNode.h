#pragma once
#include "QuadricInstance.h"
#include "Transform.h"
#include <vector>

class QuadricRenderer;

class SceneNode
{
	Transform m_Transform;
	std::vector<QuadricInstance*> m_pElements;
	std::vector<SceneNode*> m_pChildNodes;
public:
	~SceneNode();
	SceneNode() = default;
	SceneNode(const SceneNode&) = delete;
	SceneNode(SceneNode&&) = delete;
	SceneNode& operator=(const SceneNode&) = delete;
	SceneNode& operator=(SceneNode&&) = delete;

	void AddElement(QuadricInstance* pElement);
	void RemoveElement(QuadricInstance* pElement);

	void AddNode(SceneNode* pNode);
	void RemoveNode(SceneNode* pNode);

	void RenderImGui();
	void Render(QuadricRenderer* pQRenderer) const;
};