#pragma once
#include "DX12.h"
#include "ImGuiRenderer.h"
#include "QuadricRenderer.h"
#include "QuadricGeometry.h"
#include <vector>
#include "QuadricInstance.h"

class Mouse;
class Window;
class Camera;


class Editor
{
private:
	Window* m_pWindow;
	Camera* m_pCamera;
	DX12 m_DX12;
	ImGuiRenderer m_ImGuiRenderer;
	QuadricRenderer m_QRenderer;
private:
	std::vector<QuadricGeometry*> m_Geometry;
	std::vector<QuadricInstance> m_Instances;

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