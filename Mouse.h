#pragma once
#include <wtypes.h>
#include "Window.h"

enum class ButtonState {Up, Pressed, Down, Released};

class Mouse : public InputListener
{
public:
	enum class Button { Left, Middle, Right };
private:
	bool m_Capturing = false;
	POINT m_PrevMousePos;
	POINT m_MousePos;
	ButtonState m_MouseButtons[3];
	void CaptureButton(HWND hwnd, Button button, ButtonState state);
protected:
	virtual void HandleInput(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
	void Update();
	ButtonState GetMouseButton(Button button) const { return m_MouseButtons[int(button)]; };
	POINT GetMousePos() const { return m_MousePos; }
	POINT GetMouseDelta() const { return POINT{ m_MousePos.x - m_PrevMousePos.x, m_MousePos.y - m_PrevMousePos.y }; }
};