#pragma once

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "Window.h"
#include "DX12.h"
#include "d3dx12.h"
#include "QuadricRenderer.h"
#include "Mouse.h"
#include <chrono>
#include <iostream>
#include "Editor.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
	try
	{
		//setup Console
		AllocConsole();
		FILE* pDummy;
		freopen_s(&pDummy, "CONIN$", "r", stdin);
		freopen_s(&pDummy, "CONOUT$", "w", stderr);
		freopen_s(&pDummy, "CONOUT$", "w", stdout);

		Window window{ hInstance, 1600, 900 };
		Mouse mouse{};
		Editor editor{&window, &mouse};
		window.AddListener(&mouse);

		editor.Initialize();

		//LOOP
		MSG msg = {};
		auto start = std::chrono::high_resolution_clock::now();
		float passed = 0.0f;
		int framectr = 0;

		float totalTime = 0.0f;
		while (msg.message != WM_QUIT)
		{
			mouse.Update();

			while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}

			//FRAMECOUNTER

			auto end = std::chrono::high_resolution_clock::now();
			float delta = (float)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1'000'000.0f;
			start = end;
			framectr++;
			passed += delta;
			totalTime += delta;
			if (passed > 1.0f)
			{
				passed -= 1.0f;
				std::cout << "FPS: " << framectr << std::endl;
				framectr = 0;
			}

			editor.Update(delta);

			editor.Render();
		}
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
	catch (std::wstring& str)
	{
		MessageBox(nullptr, str.c_str(), L"Error", MB_OK);
		return 0;
	}
	return 0;
}