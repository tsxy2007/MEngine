#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include "Transform.h"
#include "World.h"
using namespace DirectX;
void Transform::SetRotation(XMVECTOR quaternion)
{
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quaternion);
	rotationMatrix = XMMatrixTranspose(rotationMatrix);
	XMStoreFloat3(&right, rotationMatrix.r[0]);
	XMStoreFloat3(&up, rotationMatrix.r[1]);
	XMStoreFloat3(&forward, rotationMatrix.r[2]);
}
void Transform::SetPosition(XMFLOAT3 position)
{
	this->position = position;
}

Transform::Transform(World* world):
	MObject(), world(world)
{
	worldIndex = world->allTransformsPtr.size();
	world->allTransformsPtr.emplace_back(this);
}

void Transform::SetLocalScale(XMFLOAT3 localScale)
{
	this->localScale = localScale;
}

XMMATRIX Transform::GetLocalToWorldMatrix()
{
	XMMATRIX target;
	XMVECTOR vec = { right.x, right.y, right.z, 0 };
	vec *= this->localScale.x;
	target.r[0] = vec;
	vec = { up.x, up.y, up.z, 0 };
	vec *= this->localScale.y;
	target.r[1] = vec;
	vec = { forward.x, forward.y, forward.z, 0 };
	vec *= this->localScale.z;
	target.r[2] = vec;
	target.r[3] = { position.x, position.y, position.z, 1 };
	return target;
}

XMMATRIX Transform::GetWorldToLocalMatrix()
{
	XMMATRIX localToWorld = GetLocalToWorldMatrix();
	XMVECTOR det = XMMatrixDeterminant(localToWorld);
	return XMMatrixInverse(&det, localToWorld);
}

Transform::~Transform()
{
	for (auto ite = allComponents.begin(); ite != allComponents.end(); ++ite)
	{
		(*ite)->transform = nullptr;
		(*ite)->Destroy();
	}
	allComponents.clear();
	auto&& ite = world->allTransformsPtr.end() - 1;
	(*ite)->worldIndex = worldIndex;
	world->allTransformsPtr[worldIndex] = *ite;
	world->allTransformsPtr.erase(ite);
}