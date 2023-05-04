#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "d3dx12.h"

class Window;

class DX12
{
public:
	struct Graphics;
	struct Copy;
private:
	Microsoft::WRL::ComPtr<ID3D12Device2> m_Device;
	Window* m_pWindow;
	Graphics* m_pGraphics;
	Copy* m_pCopy;
public:
	DX12(Window* pWindow);
	~DX12();
	inline Graphics* GetGraphicsInterface() const { return m_pGraphics; }
	inline Copy* GetCopyInterface() const { return m_pCopy; }
	inline ID3D12Device2* GetDevice() const { return m_Device.Get(); }
	inline Window* GetWindow() const { return m_pWindow; }
	void Present();
	void NewFrame();
};

struct DX12::Graphics
{
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	//
	static constexpr UINT k_numBackBuffers = 2;
	int currentRT = 0;

	UINT64 cpuFence[k_numBackBuffers];
	Microsoft::WRL::ComPtr<ID3D12Fence> gpuFence[k_numBackBuffers];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator[k_numBackBuffers];

	Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[k_numBackBuffers];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	Graphics(ID3D12Device2* pDevice, IDXGIFactory4* pFactory, Window* pWindow);
	void WaitForFence(int index);
	void Flush();
	ID3D12Resource* GetCurrentRenderTarget() { return renderTargets[currentRT].Get(); }
	~Graphics() { Flush(); }
};

struct DX12::Copy
{
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

	struct Buffer
	{
		static constexpr size_t k_numBuffers = 2; // 2 == double buffered

		UINT64 cpuFence[k_numBuffers];
		Microsoft::WRL::ComPtr<ID3D12Fence> gpuFence[k_numBuffers];

	};
};


