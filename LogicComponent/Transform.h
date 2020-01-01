#pragma once
#include <DirectXMath.h>
#include "../Common/MObject.h"
#include "../LogicComponent/Component.h"
class World;
class Transform : public MObject
{
	friend class Component;
private:
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 forward;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 localScale;
	DirectX::XMFLOAT3 position;
	std::vector<Component*> allComponents;
	UINT worldIndex;
	World* world;
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
	Transform(World* world);
	~Transform();
};
