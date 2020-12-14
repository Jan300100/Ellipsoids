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
#include "FreeCamera.h"
#include <ImGuiRenderer.h>


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

	Window window{ hInstance, 1600, 900 };

	DX12 dx12{&window};

	ImGuiRenderer imguiRenderer{ dx12.GetDevice(), window.GetHandle() };
	window.AddListener(&imguiRenderer);

	Mouse mouse{};
	window.AddListener(&mouse);

	Camera* pCamera = new FreeCamera{ &window, &mouse, {0,0,-4.f}, {0,0,0} };

	QuadricRenderer renderer{ &dx12, pCamera };

	Quadric e2{};
	e2.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	e2.color = DirectX::XMFLOAT3{ 0.87f,0.7f,0.6f };
	e2.position = {0,0,0};

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

		

		imguiRenderer.NewFrame();

		ImGui::Begin("Ellipsoid");
		ImGui::ColorPicker3("Color: ", &e2.color.x);
		ImGui::End();

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

		////e2.rollPitchYaw.y += delta * 2;
		//e2.position.y = sin(totalTime);

		pCamera->Update(delta);

		dx12.NewFrame();
		renderer.RenderStart();
		renderer.Render(e2);
		renderer.RenderFinish();
		imguiRenderer.Render(dx12.GetPipeline()->commandList.Get());
		dx12.Present();
	}

	return 0;
}