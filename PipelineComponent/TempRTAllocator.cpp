#include "TempRTAllocator.h"



TempRTAllocator::TempRTAllocator()
{
	usingRT.Reserve(30);
	usingRT.Reserve(30);
}


TempRTAllocator::~TempRTAllocator()
{
	for (int i = 0; i < usingRT.values.size(); ++i)
	{
		auto& v = usingRT.values[i];
		v.value.rt->Destroy();
	}
	for (int i = 0; i < waitingRT.values.size(); ++i)
	{
		auto& v = waitingRT.values[i];
		v.value.rt->Destroy();
	}
}
