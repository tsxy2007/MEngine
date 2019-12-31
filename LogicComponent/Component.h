#pragma once
#include "../Common/MObject.h"
#include "../Common/d3dUtil.h"
class Transform;
class Component : public MObject
{
	friend class Transform;
private:
	UINT componentIndex;
protected:
	Transform* transform;
public:
	Component(Transform* trans);
	virtual ~Component();
};