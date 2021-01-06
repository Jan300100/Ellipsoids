

#include "ImGuiRenderer.h"
//
#pragma warning(push, 0)
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#pragma warning(pop)
//
#include <wrl.h>
#include <d3d12.h>
#include "DX12.h"
#include <iostream>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool ImGuiRenderer::HandleInput(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam);
	return (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse);
}

ImGuiRenderer::ImGuiRenderer(ID3D12Device* pDevice, HWND hwnd)
{
	///IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiWindowFlags_AlwaysAutoResize;      
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_SrvDescHeap)) == S_OK)
	{
		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX12_Init(pDevice, 2,
			DXGI_FORMAT_R8G8B8A8_UNORM, m_SrvDescHeap.Get(),
			m_SrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			m_SrvDescHeap->GetGPUDescriptorHandleForHeapStart());
	}
}

ImGuiRenderer::~ImGuiRenderer()
{
	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiRenderer::Render(ID3D12GraphicsCommandList* commandList)
{
	ImGui::Render();
	auto heap = m_SrvDescHeap.Get();
	commandList->SetDescriptorHeaps(1, &heap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void ImGuiRenderer::NewFrame()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}