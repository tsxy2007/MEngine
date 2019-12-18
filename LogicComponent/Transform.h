#pragma once
#include <DirectXMath.h>
class Transform
{
private:
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 forward;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 localScale;
	DirectX::XMFLOAT3 position;
public:
	DirectX::XMFLOAT3 GetPosition() const { return position; }
	DirectX::XMFLOAT3 GetForward() const { return forward; }
	DirectX::XMFLOAT3 GetRight() const { return right; }
	DirectX::XMFLOAT3 GetUp() const { return up; }
	DirectX::XMFLOAT3 GetLocalScale() const { return localScale; }
	void SetRotation(DirectX::XMVECTOR quaternion);
	void SetPosition(DirectX::XMFLOAT3 position);
	void SetLocalScale(DirectX::XMFLOAT3 localScale);
	DirectX::XMMATRIX GetLocalToWorldMatrix();
	DirectX::XMMATRIX GetWorldToLocalMatrix();
};
