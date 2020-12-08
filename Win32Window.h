#pragma once
#include "Helpers.h"
#include <cstdint>

template<typename T>
struct Dimensions
{
	T width, height;
};

class Win32Window
{
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
	void SetFullscreen(bool fullscreen);
public:
	Win32Window(HINSTANCE hInstance, uint32_t width = 1280, uint32_t height = 720);
	Win32Window() = delete;
	~Win32Window() = default;
	Win32Window(const Win32Window&) = delete;
	Win32Window(Win32Window&&) = delete;
	Win32Window& operator=(const Win32Window&) = delete;
	Win32Window& operator=(Win32Window&&) = delete;
public:
	HWND GetHandle();
	Dimensions<uint32_t> GetDimensions();
};