#pragma once

struct EditQuadric
{
	DirectX::XMFLOAT4X4 equation;
	Transform transform;
	DirectX::XMFLOAT3 color = { 1,1,1 };
	Quadric ToQuadric() { return Quadric{ equation, transform.GetWorld(), color }; }
};