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

		Window window{ hInstance, 1280, 720 };
		Mouse mouse{};
		Editor editor{&window, &mouse};
		window.AddListener(&mouse);

		editor.Initialize();

		//LOOP
		MSG msg = {};
		auto start = std::chrono::high_resolution_clock::now();

		static float Timepassed = 0;
		static uint64_t numFrames = 0;
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

			std::cout << 1.f / delta << '\r';

			Timepassed += delta;
			numFrames++;

			editor.Frame(delta);
		}
		std::wstring output = L"AVERAGE ms: " + std::to_wstring(Timepassed * 1000 / numFrames) + L"\n";
		OutputDebugString(output.c_str());
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