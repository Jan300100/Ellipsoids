#pragma once

#include <wrl.h>
#include "Window.h"
struct ID3D12Device;
struct ID3D12DescriptorHeap;
struct ID3D12GraphicsCommandList;

class ImGuiRenderer : public InputListener
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SrvDescHeap;
	virtual bool HandleInput(HWND, UINT, WPARAM, LPARAM) override;
public:
	ImGuiRenderer(ID3D12Device* pDevice, HWND hwnd, UINT numFramesInFlight);
	~ImGuiRenderer();
	void RenderUI(ID3D12GraphicsCommandList* commandList);
	void NewFrame();
};