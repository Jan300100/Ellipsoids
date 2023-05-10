#include "DX12.h"
#include "Helpers.h"
#include "Window.h"

#ifndef USE_PIX
#define USE_PIX
#endif
#include <pix3.h>

using namespace Microsoft::WRL;


DX12::DX12(Window* pWindow)
	:m_pWindow{pWindow}
{
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	//FACTORY
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

	//DEVICE
		// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_pDevice));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_pDevice)));
	}

	//GRAPHICS
	//********
	m_pGraphics = new Graphics{ m_pDevice.Get(),factory.Get(), m_pWindow };
	m_pCompute = new Compute{ m_pDevice.Get() };
}

DX12::~DX12()
{
	delete m_pGraphics;
	delete m_pCompute;
}

void DX12::Graphics::Present()
{
	//// Swap the back and front buffers
	m_SwapChain->Present(0, 0);

	ThrowIfFailed(m_pDevice->GetDeviceRemovedReason());
}

void DX12::Graphics::NextFrame()
{
	m_CurrentRT = static_cast<IDXGISwapChain3*>(m_SwapChain.Get())->GetCurrentBackBufferIndex();
	// make sure this backbuffer is not in flight anymore.
	WaitForFence(m_CurrentRT);

	// new Frame
	ThrowIfFailed(m_CommandAllocator[m_CurrentRT]->Reset());
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator[m_CurrentRT].Get(), nullptr));

	// Indicate a state transition on the resource usage.
	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentRenderTarget(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_CommandList->ResourceBarrier(1, &transition);

	// Clear the back buffer
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_CurrentRT,
		m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));


	FLOAT col[4]{ 0.4f,0.4f,0.4f,1 };
	m_CommandList->ClearRenderTargetView(handle, col, 0, nullptr);
	m_CommandList->OMSetRenderTargets(1, &handle, FALSE, NULL);
}

DX12::Graphics::Graphics(Microsoft::WRL::ComPtr<ID3D12Device2> pDevice, IDXGIFactory4* pFactory, Window* pWindow)
{
	m_pDevice = pDevice;

	//FENCES
	for (size_t i = 0; i < k_numBackBuffers; i++)
	{
		m_CpuFence[i] = 0;
		ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&m_GpuFence[i])));

		ThrowIfFailed(pDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(m_CommandAllocator[i].GetAddressOf())));
	}


	//COMMAND OBJECTS
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

	

	ThrowIfFailed(pDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_CommandAllocator[m_CurrentRT].Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(m_CommandList.GetAddressOf())));


	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	m_CommandList->Close();



	//Swapchain
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = pWindow->GetDimensions().width;
	sd.BufferDesc.Height = pWindow->GetDimensions().height;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = k_numBackBuffers;
	sd.OutputWindow = pWindow->GetHandle();
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	
	ThrowIfFailed(pFactory->CreateSwapChain(
		m_CommandQueue.Get(),
		&sd,
		m_SwapChain.GetAddressOf()));

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	ZeroMemory(&rtvHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	rtvHeapDesc.NumDescriptors = k_numBackBuffers;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(m_RtvHeap.GetAddressOf())));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());

	std::wstring name = L"RenderTarget";
	for (int i = 0; i < k_numBackBuffers; i++)
	{
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i])));
		pDevice->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		m_RenderTargets[i]->SetName((name + std::to_wstring(i)).c_str());
	}
}

void DX12::Graphics::Execute()
{
	// Transition to PRESENT state.
	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentRenderTarget(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_CommandList->ResourceBarrier(1, &transition);
	// Done recording commands.
	ThrowIfFailed(m_CommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	m_CpuFence[m_CurrentRT]++;
	m_CommandQueue->Signal(m_GpuFence[m_CurrentRT].Get(), m_CpuFence[m_CurrentRT]);
}

void DX12::Graphics::Flush()
{
	m_CpuFence[m_CurrentRT]++;

	m_CommandQueue->Signal(m_GpuFence[m_CurrentRT].Get(), m_CpuFence[m_CurrentRT]);
	WaitForFence(m_CurrentRT);
}

void DX12::Graphics::WaitForFence(int index)
{
	PIXScopedEvent(0, "WaitForFence: %d", index);
	if (m_GpuFence[index]->GetCompletedValue() < m_CpuFence[index])
	{
		HANDLE e = CreateEventEx(nullptr, FALSE, false, EVENT_ALL_ACCESS);
		m_GpuFence[index]->SetEventOnCompletion(m_CpuFence[index], e);
		WaitForSingleObject(e, INFINITE);
		CloseHandle(e);
	}
}

DX12::Compute::Compute(Microsoft::WRL::ComPtr<ID3D12Device2> pDevice)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

	ThrowIfFailed(pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		IID_PPV_ARGS(m_CommandAllocator.GetAddressOf())));

	ThrowIfFailed(pDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		m_CommandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(m_CommandList.GetAddressOf())));

	m_CommandList->Close();

	m_CpuFence = 0;
	ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_GpuFence)));
}

void DX12::Compute::Execute()
{
	ThrowIfFailed(m_CommandList->Close());
	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	m_CpuFence++;
	m_CommandQueue->Signal(m_GpuFence.Get(), m_CpuFence);
}

void DX12::Compute::NextFrame()
{
	WaitForFence();
	ThrowIfFailed(m_CommandAllocator->Reset());

	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));
}

void DX12::Compute::WaitForFence()
{
	PIXScopedEvent(0, "Compute::WaitForFence");
	if (m_GpuFence->GetCompletedValue() < m_CpuFence)
	{
		HANDLE e = CreateEventEx(nullptr, FALSE, false, EVENT_ALL_ACCESS);
		m_GpuFence->SetEventOnCompletion(m_CpuFence, e);
		WaitForSingleObject(e, INFINITE);
		CloseHandle(e);
	}
}

void DX12::Compute::Flush()
{
	m_CpuFence++;
	m_CommandQueue->Signal(m_GpuFence.Get(), m_CpuFence);
	WaitForFence();
}
