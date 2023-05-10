#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "d3dx12.h"

class Window;

class DX12
{
public:
	class Graphics;
	class Compute;
private:
	Microsoft::WRL::ComPtr<ID3D12Device2> m_pDevice;
	Window* m_pWindow;
	Graphics* m_pGraphics;
	Compute* m_pCompute;
public:
	DX12(Window* pWindow);
	~DX12();
	inline Graphics* GetGraphicsInterface() const { return m_pGraphics; }
	inline Compute* GetComputeInterface() const { return m_pCompute; }
	inline ID3D12Device2* GetDevice() const { return m_pDevice.Get(); }
	inline Window* GetWindow() const { return m_pWindow; }
};

class DX12::Graphics
{
public:
	static constexpr UINT k_numBackBuffers = 2;
public:
	Graphics(Microsoft::WRL::ComPtr<ID3D12Device2> pDevice, IDXGIFactory4* pFactory, Window* pWindow);
	void Execute();
	void Present();
	void NextFrame();
	void WaitForFence();
	void Flush();
	ID3D12GraphicsCommandList* GetCommandList() { return m_CommandList.Get(); }
	ID3D12Resource* GetCurrentRenderTarget() { return m_RenderTargets[m_CurrentRT].Get(); }
	~Graphics() { Flush(); }
private:
	void WaitForFence(int index);

	Microsoft::WRL::ComPtr<ID3D12Device2> m_pDevice;

	Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;

	int m_CurrentRT = 0;

	UINT64 m_CpuFence[k_numBackBuffers];
	Microsoft::WRL::ComPtr<ID3D12Fence> m_GpuFence[k_numBackBuffers];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator[k_numBackBuffers];

	Microsoft::WRL::ComPtr<ID3D12Resource> m_RenderTargets[k_numBackBuffers];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap;

};

// Add this new struct
class DX12::Compute
{
public:
	Compute(Microsoft::WRL::ComPtr<ID3D12Device2> pDevice);
	void Execute();
	void NextFrame();
	void WaitForFence();
	void Flush();
private:
	Microsoft::WRL::ComPtr<ID3D12Device2> m_pDevice;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
	UINT64 m_CpuFence;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_GpuFence;
};