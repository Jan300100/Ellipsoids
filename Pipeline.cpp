#include "Pipeline.h"
#include "Helpers.h"
#include "d3dx12.h"
#include "Win32Window.h"

void Pipeline::Render(ComPtr<ID3D12Resource> texture)
{
	ThrowIfFailed(m_CommandAllocator->Reset());
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));


	// Indicate a state transition on the resource usage.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTarget[m_CurrentRT].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_CurrentRT,
		m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	FLOAT col[4]{ 255,0,0,1 };
	m_CommandList->ClearRenderTargetView(handle, col, 0, nullptr);

	// Specify the buffers we are going to render to.
	m_CommandList->OMSetRenderTargets(1, &handle, true, nullptr);

	//COPY
	// Prepare to copy blurred output to the back buffer.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTarget[m_CurrentRT].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST));

	m_CommandList->CopyResource(m_RenderTarget[m_CurrentRT].Get(), texture.Get());

	// Transition to PRESENT state.
	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTarget[m_CurrentRT].Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));


	// Done recording commands.
	ThrowIfFailed(m_CommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);



	// Swap the back and front buffers
	ThrowIfFailed(m_SwapChain->Present(0, 0));

	m_CurrentRT = (m_CurrentRT + 1) % m_RTVCount;

	Flush();
}

void Pipeline::Flush()
{
	m_CPUFence++;

	m_CommandQueue->Signal(m_GPUFence.Get(), m_CPUFence);

	if (m_GPUFence->GetCompletedValue() < m_CPUFence)
	{
		HANDLE e = CreateEventEx(nullptr, FALSE, false, EVENT_ALL_ACCESS);
		m_GPUFence->SetEventOnCompletion(m_CPUFence, e);
		WaitForSingleObject(e, INFINITE);
		CloseHandle(e);
	}
}

Pipeline::~Pipeline()
{
	if (m_Device != nullptr)
	{
		Flush();
	}
}

Pipeline::Pipeline(Win32Window* pWindow)
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
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_Factory)));

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
		ThrowIfFailed(m_Factory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_Device)));
	}

	//FENCE
	ThrowIfFailed(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&m_GPUFence)));

	//COMMAND OBJECTS
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

	ThrowIfFailed(m_Device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(m_CommandAllocator.GetAddressOf())));

	ThrowIfFailed(m_Device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_CommandAllocator.Get(), // Associated command allocator
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
	sd.BufferCount = m_RTVCount;
	sd.OutputWindow = pWindow->GetHandle();
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	ThrowIfFailed(m_Factory->CreateSwapChain(
		m_CommandQueue.Get(),
		&sd,
		m_SwapChain.GetAddressOf()));

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	ZeroMemory(&rtvHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	rtvHeapDesc.NumDescriptors = m_RTVCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_Device->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(m_RtvHeap.GetAddressOf())));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < m_RTVCount; i++)
	{
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTarget[i])));
		m_Device->CreateRenderTargetView(m_RenderTarget[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	}

}
