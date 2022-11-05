#pragma once


#include "DX12.h"
#include "ImGuiRenderer.h"
#include "QuadricRenderer.h"
#include "QuadricGeometry.h"
#include <vector>
#include <map>
#include "QuadricInstance.h"
#include "EditQuadric.h"
#include "EditableGeometry.h"

class Mouse;
class Window;
class Camera;
class SceneNode;

class Editor
{
private:
	Window* m_pWindow;
	Camera* m_pCamera;
	DX12 m_DX12;
	ImGuiRenderer m_ImGuiRenderer;
	QuadricRenderer m_QRenderer;
private:
	std::map<std::string, EditableGeometry> m_Geometry;
	std::vector<SceneNode*> m_Prefabs;
	SceneNode* m_pCurrentScene;
	SceneNode* m_pGeometryEditor;
	QuadricInstance* m_pEditResult;
public:
	Editor() = delete;
	~Editor();
	Editor(const Editor&) = delete;
	Editor(Editor&&) = delete;
	Editor& operator=(const Editor&) = delete;
	Editor& operator=(Editor&&) = delete;
	Editor(Window* pWindow, Mouse* pMouse);
	void Initialize();
	void Update(float dt);
	void Render();
};