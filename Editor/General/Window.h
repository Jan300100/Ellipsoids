#pragma once
#include "Helpers.h"
#include <cstdint>

class InputListener
{
	InputListener* m_pNextListener = nullptr;
	InputListener* m_pPreviousListener = nullptr;
protected:
	virtual bool HandleInput(HWND, UINT, WPARAM, LPARAM) { return false; }
public:
	virtual ~InputListener();
	void AddListener(InputListener* pListener);
	void RemoveListener(InputListener* pListener);
	void Input(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

class Window : public InputListener
{
private:
	bool m_HasTitleBar = true;
	Dimensions<uint32_t> m_Dimensions;
	// Window handle.
	HWND m_Hwnd;
	// Window rectangle (used to toggle fullscreen state).
	RECT m_WindowRect;
	bool m_Fullscreen = false;
private:
	HWND ConstructWindow(const wchar_t* windowClassName, HINSTANCE hInst,
		const wchar_t* windowTitle, uint32_t width, uint32_t height);
	void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName);
	static LRESULT CALLBACK WndProcStatic(HWND, UINT, WPARAM, LPARAM);
	LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
public:
	Window(HINSTANCE hInstance, uint32_t width = 1280, uint32_t height = 720, bool hasTitleBar = true);
	Window() = delete;
	~Window() = default;
	Window(const Window&) = delete;
	Window(Window&&) = delete;
	Window& operator=(const Window&) = delete;
	Window& operator=(Window&&) = delete;
	float AspectRatio() const;
	void SetFullscreen(bool fullscreen);
public:
	HWND GetHandle();
	Dimensions<uint32_t> GetDimensions();
};

