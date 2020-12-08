#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <string>


using namespace Microsoft::WRL;

class Pipeline;
class Win32Window;
class Compute
{
	Pipeline* m_pPipeline;
	
	ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;

	//Buffers
	ComPtr<ID3D12Resource> m_InputBuffer;
	ComPtr<ID3D12Resource> m_InputBufferUpload;

	ComPtr<ID3D12Resource> m_OutputBuffer;
	ComPtr<ID3D12Resource> m_OutputBufferReadback;

	ComPtr<ID3D12Resource> m_OutputTexture;

	//Root Signature
	ComPtr<ID3D12RootSignature> m_RootSignature;

	//Shader
	ComPtr<ID3DBlob> m_Shader;

	//PSO
	ComPtr<ID3D12PipelineState> m_Pso;

	Win32Window* m_pWindow;
public:
	ComPtr<ID3D12Resource> GetOutput() { return m_OutputTexture; }

	Compute(Pipeline* pPipeline, Win32Window* pWindow);
	void CreateOutputTexture();
	void Execute();

};