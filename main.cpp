#pragma once

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <cstdio>
#include <wtypes.h>

#include "Pipeline.h"
#include "Compute.h"

#include <iostream>

#include "Win32Window.h"
#include <time.h>


int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
#ifdef _DEBUG
	//setup Console
	AllocConsole();
	FILE* pDummy;
	freopen_s(&pDummy, "CONIN$", "r", stdin);
	freopen_s(&pDummy, "CONOUT$", "w", stderr);
	freopen_s(&pDummy, "CONOUT$", "w", stdout);
#endif

	srand(time(nullptr));

	Win32Window window{ hInstance, 640, 480 };
	Pipeline directx{&window };
	Compute compute{ &directx, &window };

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		compute.Execute();
		directx.Render(compute.GetOutput());
	}
}

