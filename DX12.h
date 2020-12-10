#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
class Window;

class DX12
{
public:
	struct Pipeline;
private:
	Microsoft::WRL::ComPtr<ID3D12Device2> m_Device;
	Window* m_pWindow;
	Pipeline* m_pGraphics;
public:
	DX12(Window* pWindow);
	~DX12();
	inline Pipeline* GetPipeline() const { return m_pGraphics; }
	inline ID3D12Device2* GetDevice() const { return m_Device.Get(); }
	inline Window* GetWindow() const { return m_pWindow; }
};

struct DX12::Pipeline
{
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	UINT64 cpuFence = 0;
	Microsoft::WRL::ComPtr<ID3D12Fence> gpuFence;
	//
	static const int rtvCount = 2;
	int currentRT = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[rtvCount];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	Pipeline(ID3D12Device2* pDevice, IDXGIFactory4* pFactory, Window* pWindow);
	void Flush();
	ID3D12Resource* GetCurrentRenderTarget() { return renderTargets[currentRT].Get(); }
	~Pipeline() { Flush(); }
};
