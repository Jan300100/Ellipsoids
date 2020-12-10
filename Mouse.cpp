#include "Mouse.h"
#include <windowsx.h>


void Mouse::CaptureButton(HWND hwnd, Button button, ButtonState state)
{
	switch (state)
	{
	case ButtonState::Pressed:
		if (m_MouseButtons[(int)button] == ButtonState::Pressed)
		{
			m_MouseButtons[(int)button] = ButtonState::Down;
		}
		else
		{
			m_MouseButtons[(int)button] = ButtonState::Pressed;
		}
		if (!m_Capturing)
		{
			SetCapture(hwnd);
			m_Capturing = true;
		}
		break;
	case ButtonState::Released:
		if (m_MouseButtons[(int)button] == ButtonState::Released)
		{
			m_MouseButtons[(int)button] = ButtonState::Up;
		}
		else
		{
			m_MouseButtons[(int)button] = ButtonState::Released;
		}
		if (m_Capturing)
		{
			ReleaseCapture();
			m_Capturing = false;
		}
		break;
	case ButtonState::Down:
	case ButtonState::Up:
	default:
		m_MouseButtons[(int)button] = state;
		break;
	}

}

void Mouse::HandleInput(HWND hwnd, UINT message, WPARAM, LPARAM lParam)
{
	switch (message)
	{
	case WM_LBUTTONDOWN:
		CaptureButton(hwnd, Button::Left, ButtonState::Pressed);
		break;
	case WM_LBUTTONUP:
		CaptureButton(hwnd, Button::Left, ButtonState::Released);
		break;
	case WM_RBUTTONDOWN:
		CaptureButton(hwnd, Button::Right, ButtonState::Pressed);
		break;
	case WM_RBUTTONUP:
		CaptureButton(hwnd, Button::Right, ButtonState::Released);
		break;
	case WM_MBUTTONDOWN:
		CaptureButton(hwnd, Button::Middle, ButtonState::Pressed);
		break;
	case WM_MBUTTONUP:
		CaptureButton(hwnd, Button::Middle, ButtonState::Released);
		break;
	case WM_MOUSEMOVE:
		m_MousePos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		break;
	}
}

void Mouse::Update()
{
	m_PrevMousePos = m_MousePos;
	for (ButtonState& state : m_MouseButtons)
	{
		if (state == ButtonState::Pressed)
		{
			state = ButtonState::Down;
		}
		if (state == ButtonState::Released)
		{
			state = ButtonState::Up;
		}
	}
}
