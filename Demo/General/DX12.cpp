#include "DX12.h"
#include "Helpers.h"
#include "Window.h"

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
		IID_PPV_ARGS(&m_Device));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_Device)));
	}

	//GRAPHICS
	//********
	m_pGraphics = new Pipeline{ m_Device.Get(),factory.Get(), m_pWindow };

}

DX12::~DX12()
{
	delete m_pGraphics;
}

void DX12::Present()
{
	// Transition to PRESENT state.
	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(m_pGraphics->GetCurrentRenderTarget(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_pGraphics->commandList->ResourceBarrier(1, &transition);


	// Done recording commands.
	ThrowIfFailed(m_pGraphics->commandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { m_pGraphics->commandList.Get() };
	m_pGraphics->commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(m_pGraphics->swapChain->Present(0, 0));

	m_pGraphics->currentRT = static_cast<IDXGISwapChain3*>(m_pGraphics->swapChain.Get())->GetCurrentBackBufferIndex();
	m_pGraphics->Flush(); //wait for gpu to finish (== not ideal)
}

void DX12::NewFrame()
{
	ThrowIfFailed(m_pGraphics->commandAllocator->Reset());
	ThrowIfFailed(m_pGraphics->commandList->Reset(m_pGraphics->commandAllocator.Get(), nullptr));

	// Indicate a state transition on the resource usage.
	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(m_pGraphics->GetCurrentRenderTarget(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pGraphics->commandList->ResourceBarrier(1, &transition);

	// Clear the back buffer
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_pGraphics->rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_pGraphics->currentRT,
		m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));


	FLOAT col[4]{ 0.4f,0.4f,0.4f,1 };
	m_pGraphics->commandList->ClearRenderTargetView(handle, col, 0, nullptr);
	m_pGraphics->commandList->OMSetRenderTargets(1, &handle, FALSE, NULL);
}

DX12::Pipeline::Pipeline(ID3D12Device2* pDevice, IDXGIFactory4* pFactory, Window* pWindow)
{
	//FENCE
	ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&gpuFence)));

	//COMMAND OBJECTS
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	ThrowIfFailed(pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(commandAllocator.GetAddressOf())));

	ThrowIfFailed(pDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(commandList.GetAddressOf())));


	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	commandList->Close();



	//Swapchain
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = pWindow->GetDimensions().width;
	sd.BufferDesc.Height = pWindow->GetDimensions().height;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = rtvCount;
	sd.OutputWindow = pWindow->GetHandle();
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	
	ThrowIfFailed(pFactory->CreateSwapChain(
		commandQueue.Get(),
		&sd,
		swapChain.GetAddressOf()));

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	ZeroMemory(&rtvHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	rtvHeapDesc.NumDescriptors = rtvCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf())));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < rtvCount; i++)
	{
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
		pDevice->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	}
}

void DX12::Pipeline::Flush()
{
	cpuFence++;

	commandQueue->Signal(gpuFence.Get(), cpuFence);

	if (gpuFence->GetCompletedValue() < cpuFence)
	{
		HANDLE e = CreateEventEx(nullptr, FALSE, false, EVENT_ALL_ACCESS);
		gpuFence->SetEventOnCompletion(cpuFence, e);
		WaitForSingleObject(e, INFINITE);
		CloseHandle(e);
	}
}


