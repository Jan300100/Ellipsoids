#include "Transform.h"

using namespace DirectX;

void Transform::CalculateMatrix()
{
    matrix = XMMatrixAffineTransformation(XMLoadFloat3(&scale), XMVectorZero()
        , XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation))
        , XMLoadFloat3(&position));
    matrix = XMMatrixInverse(nullptr, XMMatrixTranspose(matrix));
}
