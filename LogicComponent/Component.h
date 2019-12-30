#pragma once
#include "../Common/MObject.h"
class Transform;
class Component : public MObject
{
protected:
	Transform* transform;
public:
	Component(Transform* trans);
	virtual ~Component() {}
};