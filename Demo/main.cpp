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
#include "QuadricGeometry.h"
#include "Structs.h"
#include "Mouse.h"
#include <chrono>
#include "FreeCamera.h" 
#include <vector>
#include <iostream>
#include <fstream>

struct EditQuadric
{
	DirectX::XMFLOAT4X4 equation;
	Transform transform;
	DirectX::XMFLOAT3 color = { 1,1,1 };
	Quadric ToQuadric() { return Quadric{ equation, transform.GetWorld(), color }; }
};

float randf() { return rand() % 10'000 / 10'000.0f; }

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
		window.AddListener(&mouse);

		DX12 dx12{ &window };
		FreeCamera cam{&window, &mouse};
		cam.Offset({ 0,0,-2.0f });
		QuadricRenderer renderer{ dx12.GetDevice(), window.GetDimensions().width, window.GetDimensions().height };
		renderer.SetProjectionVariables(cam.GetFOV(), window.AspectRatio(), cam.GetNearPlane(), cam.GetFarPlane());

		dx12.GetPipeline()->commandAllocator->Reset();
		dx12.GetPipeline()->commandList->Reset(dx12.GetPipeline()->commandAllocator.Get(), nullptr);
		renderer.Initialize(dx12.GetPipeline()->commandList.Get());
		dx12.GetPipeline()->commandList->Close();
		ID3D12CommandList* list = dx12.GetPipeline()->commandList.Get();
		dx12.GetPipeline()->commandQueue->ExecuteCommandLists(1, &list);
		dx12.GetPipeline()->Flush();

		dx12.GetPipeline()->commandAllocator->Reset();
		dx12.GetPipeline()->commandList->Reset(dx12.GetPipeline()->commandAllocator.Get(), nullptr);
		renderer.SetRendererSettings(dx12.GetPipeline()->commandList.Get(), 512, { 128, 128 }, 64);
		QuadricGeometry benchmarkMesh{ "BenchmarkMesh" };

		EditQuadric e{};
		//implicit equation
		e.equation = DirectX::XMFLOAT4X4{
				1,0,0,0,
				0,1,0,0,
				0,0,1,0,
				0,0,0,-1 };

		Quadric q{ e.ToQuadric() };
		std::vector<Quadric> quadrics{};
		quadrics.push_back(q);

		benchmarkMesh.Init(dx12.GetDevice(), dx12.GetPipeline()->commandList.Get(), quadrics, 100);

		//finish initialization
		dx12.GetPipeline()->commandList->Close();
		dx12.GetPipeline()->commandQueue->ExecuteCommandLists(1, &list);
		dx12.GetPipeline()->Flush();

		//LOOP
		MSG msg = {};
		auto start = std::chrono::high_resolution_clock::now();

		while (msg.message != WM_QUIT)
		{
			mouse.Update();

			while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}

			auto end = std::chrono::high_resolution_clock::now();
			float dt = (float)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1'000'000.0f;
			start = end;



			//UPDATE
			static UINT frameCtr = 0;
			static float ctr = 0.0f;
			ctr += dt;
			frameCtr++;
			if (ctr > 1.0f)
			{
				std::cout << frameCtr << '\r';
				ctr--;
				frameCtr = 0;
			}
			
			cam.Update(dt);
			renderer.SetViewMatrix(cam.GetView());

			renderer.Render(&benchmarkMesh);


			//RENDER
			dx12.NewFrame();
			renderer.RenderFrame(dx12.GetPipeline()->commandList.Get(), dx12.GetPipeline()->GetCurrentRenderTarget());
			dx12.Present();
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