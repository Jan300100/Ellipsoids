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
#include "EllipsoidRenderer.h"
#include "Camera.h"
#include "Mouse.h"
#include <chrono>
#include <iostream>

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
	Window window{ hInstance, 920, 720 };
	Mouse mouse{};
	window.AddListener(&mouse);

	Camera camera{ &window, &mouse, {0,0,-4.f}, {0,0,1} };
	DX12 dx12{&window};
	EllipsoidRenderer renderer{ &dx12, &camera };

	Ellipsoid e{};
	e.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	e.color = DirectX::XMFLOAT3{ 1,1,1 };

	Ellipsoid e2{};



	e2.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	e2.color = DirectX::XMFLOAT3{ 0.77f,0.64f,0 };
	e2.position = {0,0,0};
	e2.scale = {0.25f,0.8f,1.2f};
	e2.rollPitchYaw = {DirectX::XM_PIDIV4, 0 ,DirectX::XM_PIDIV4 };
	//LOOP
	MSG msg = {};
	auto start = std::chrono::high_resolution_clock::now();
	float passed = 0.0f;
	int framectr = 0;
	float totalTime = 0.0f;
	while (msg.message != WM_QUIT)
	{

		auto end = std::chrono::high_resolution_clock::now();
		float delta = (float)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1'000'000.0f;
		if (delta > 0.01f) delta = 0.01f;
		start = end;
		framectr++;
		passed += delta;
		if (passed > 1.0f)
		{
			passed -= 1.0f;
			std::cout << "FPS: " << framectr << std::endl;
			framectr = 0;
		}

		totalTime += delta;

		e2.rollPitchYaw.y += delta * 2;
		e2.position.y = sin(totalTime);

		mouse.Update();
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		camera.Update(delta);
		//renderer.RenderStart(); 
		//renderer.Render(e);
		//renderer.RenderFinish();

		renderer.RenderStart();
		renderer.Render(e2);
		renderer.RenderFinish();
	}
}