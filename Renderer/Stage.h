#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxcapi.h>

class QuadricRenderer;
class QuadricGeometry;
namespace Stage
{
	class Stage
	{
	protected:
		Microsoft::WRL::ComPtr<IDxcBlob> m_Shader;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso;
		bool m_Initialized = false;
	public:
		virtual ~Stage() = default;
		virtual void Init(QuadricRenderer* pRenderer) = 0;
	};
}