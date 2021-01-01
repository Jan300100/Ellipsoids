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
#include "Structs.h"
#include "QuadricMesh.h"

int main() {
	return 0;
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
	//setup Console
	AllocConsole();
	FILE* pDummy;
	freopen_s(&pDummy, "CONIN$", "r", stdin);
	freopen_s(&pDummy, "CONOUT$", "w", stderr);
	freopen_s(&pDummy, "CONOUT$", "w", stdout);

	Window window{ hInstance, 640, 480 };

	DX12 dx12{&window};

	ImGuiRenderer imguiRenderer{ dx12.GetDevice(), window.GetHandle() };
	window.AddListener(&imguiRenderer);

	Mouse mouse{};
	window.AddListener(&mouse);

	Transform cameraTransform{};
	cameraTransform.position = { 0,4.5,-4.f };
	Camera* pCamera = new FreeCamera{ &window, &mouse, cameraTransform };

	QuadricRenderer renderer{ &dx12, pCamera };

	DirectX::XMFLOAT3 skinColor{ 1.0f,0.67f,0.45f }, tShirtColor{ 1,0,0 }, pantsColor{0,0,1}, shoeColor{0.6f,0.4f,0.1f};
	
	Quadric head{};
	head.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	head.color = skinColor;
	head.transform.scale = { 1,1,1 };
	head.transform.position = { 0,2,0 };

	Quadric body{};
	body.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	body.color = tShirtColor;
	body.transform.scale = { 1,2,1 };
	body.transform.position = { 0,0,0 };

	Quadric upperArmRight;
	upperArmRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	upperArmRight.color = tShirtColor;
	upperArmRight.transform.scale = { 0.4f,1.0f,0.4f };
	upperArmRight.transform.position = { 1.25f,0.6f,0 };
	upperArmRight.transform.rotation = {0,0,1};

	Quadric lowerArmRight;
	lowerArmRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	lowerArmRight.color = skinColor;
	lowerArmRight.transform.scale = { 0.3f,1.0f,0.3f };
	lowerArmRight.transform.position = { 1.85f,-0.6f,0 };

	Quadric handRight;
	handRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	handRight.color = skinColor;
	handRight.transform.scale = { 0.3f,0.5f,0.3f };
	handRight.transform.position = { 1.85f,-1.5f,0 };

	Quadric upperArmLeft;
	upperArmLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	upperArmLeft.color = tShirtColor;
	upperArmLeft.transform.scale = { 0.4f,1.0f,0.4f };
	upperArmLeft.transform.position = { -1.25f,0.6f,0 };
	upperArmLeft.transform.rotation = { 0,0,-1 };

	Quadric lowerArmLeft;
	lowerArmLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	lowerArmLeft.color = skinColor;
	lowerArmLeft.transform.scale = { 0.3f,1.0f,0.3f };
	lowerArmLeft.transform.position = { -1.85f,-0.6f,0 };

	Quadric handLeft;
	handLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	handLeft.color = skinColor;
	handLeft.transform.scale = { 0.3f,0.5f,0.3f };
	handLeft.transform.position = { -1.85f,-1.5f,0 };

	//leg
	Quadric upperLegRight;
	upperLegRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	upperLegRight.color = pantsColor;
	upperLegRight.transform.scale = { 0.5f,1.5f,0.5f };
	upperLegRight.transform.position = { 0.5f,-2.0f,0 };

	Quadric lowerLegRight;
	lowerLegRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	lowerLegRight.color = pantsColor;
	lowerLegRight.transform.scale = { 0.5f,1.5f,0.5f };
	lowerLegRight.transform.position = { 0.5f,-3.0f,0 };

	Quadric shoeRight;
	shoeRight.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	shoeRight.color = shoeColor;
	shoeRight.transform.scale = { 0.5f,0.3f,0.75f };
	shoeRight.transform.position = { 0.5f,-4.5f,-0.5f };

	//leg
	Quadric upperLegLeft;
	upperLegLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	upperLegLeft.color = pantsColor;
	upperLegLeft.transform.scale = { 0.5f,1.5f,0.5f };
	upperLegLeft.transform.position = { -0.5f,-2.0f,0 };

	Quadric lowerLegLeft;
	lowerLegLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	lowerLegLeft.color = pantsColor;
	lowerLegLeft.transform.scale = { 0.5f,1.5f,0.5f };
	lowerLegLeft.transform.position = {- 0.5f,-3.0f,0 };

	Quadric shoeLeft;
	shoeLeft.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	shoeLeft.color = shoeColor;
	shoeLeft.transform.scale = { 0.5f,0.3f,0.75f };
	shoeLeft.transform.position = {- 0.5f,-4.5f,-0.5f };

	std::vector<InQuadric> in{};

	in.push_back(head);
	in.push_back(body);
	in.push_back(upperArmRight);
	in.push_back(lowerArmRight);
	in.push_back(handRight);
	in.push_back(upperArmLeft);
	in.push_back(lowerArmLeft);
	in.push_back(handLeft);
	in.push_back(upperLegRight);
	in.push_back(lowerLegRight);
	in.push_back(shoeRight);
	in.push_back(upperLegLeft);
	in.push_back(lowerLegLeft);
	in.push_back(shoeLeft);

	size_t count = 1;
	std::vector< QuadricMesh> dudes{};
	for (size_t i = 0; i < count; i++)
	{
		for (size_t j = 0; j < count; j++)
		{
			dudes.emplace_back(&dx12, in);
			dudes.back().GetTransform().position.x = 5.0f * i;
			dudes.back().GetTransform().position.z = 5.0f * j;
		}
	}

	Quadric ellipsoid{};
	ellipsoid.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };


	Quadric world;
	float range = 1000;
	world.equation = DirectX::XMFLOAT4X4{
					1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,-1 };
	world.color = { 75 / 255.0f,168 / 255.0f,59 / 255.0f };
	world.transform.scale = { range,range,range };
	world.transform.position = { 0,-range,0 };

	std::vector<InQuadric> groundInput{};
	groundInput.push_back(world);
	QuadricMesh ground{ &dx12, groundInput };

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


		auto end = std::chrono::high_resolution_clock::now();
		float delta = (float)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1'000'000.0f;
		start = end;
		framectr++;
		passed += delta;
		if (passed > 1.0f)
		{
			passed -= 1.0f;
			std::cout << "FPS: " << framectr << "\t\r";
			framectr = 0;
		}

		totalTime += delta;


		pCamera->Update(delta);
		dx12.NewFrame();
		//renderer.Render(&ground);

		for (QuadricMesh& dude : dudes)
			renderer.Render(&dude);

		renderer.Render();
		imguiRenderer.Render(dx12.GetPipeline()->commandList.Get());
		dx12.Present();
	}

	return 0;
}