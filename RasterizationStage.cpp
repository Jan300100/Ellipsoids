#include "RasterizationStage.h"
#include "QuadricRenderer.h"

Stage::Rasterization::Rasterization(DX12* pDX12)
	:Stage{pDX12} 
{
}

void Stage::Rasterization::Execute(QuadricRenderer*, QuadricMesh*) const
{
}
