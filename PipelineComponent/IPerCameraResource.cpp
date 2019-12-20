#include "IPerCameraResource.h"
#include "../Common/Camera.h"
#include "../Singleton/FrameResource.h"
PerCameraData::~PerCameraData()
{
	for (auto&& ite = resources.begin(); ite != resources.end(); ite++)
	{
		delete ite->second;
	}
}