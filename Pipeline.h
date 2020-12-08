#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class Win32Window;

class Pipeline
{
	UINT64 m_CPUFence = 0;
	ComPtr<IDXGIFactory4> m_Factory;
	ComPtr<ID3D12Device2> m_Device;
	ComPtr<IDXGISwapChain> m_SwapChain;
	ComPtr<ID3D12CommandQueue> m_CommandQueue;
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;
	ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
	ComPtr<ID3D12Fence> m_GPUFence;
	static const int m_RTVCount = 2;
	int m_CurrentRT = 0;
	ComPtr<ID3D12Resource> m_RenderTarget[m_RTVCount];
	ComPtr<ID3D12DescriptorHeap> m_RtvHeap;

	Win32Window* m_pWindow;
public:

	Pipeline(Win32Window* pWindow);
	~Pipeline();

	void Render(ComPtr<ID3D12Resource> texture);

	void Flush();
	inline ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return m_CommandList; }
	inline ComPtr<ID3D12CommandQueue> GetCommandQueue() { return m_CommandQueue; }
	inline ComPtr<ID3D12CommandAllocator> GetCommandAllocator() { return m_CommandAllocator; }

	ComPtr<ID3D12Device2> GetDevice() const { return m_Device; }
};

