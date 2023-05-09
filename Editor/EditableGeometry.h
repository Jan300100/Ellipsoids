#pragma once
#include <vector>
#include "EditQuadric.h"

struct ID3D12Device2;
struct ID3D12GraphicsCommandList;
class QuadricGeometry;

struct EditableGeometry
{
	std::vector<EditQuadric> quadrics;
	void UpdateGeometry(QuadricRenderer* pRenderer, ID3D12GraphicsCommandList* pComList);
	QuadricGeometry* pGeometry;
};