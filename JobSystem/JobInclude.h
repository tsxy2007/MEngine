#pragma once
#include "JobSystem.h"
#include "JobBucket.h"
#include "JobNode.h"
template <typename Func>
JobHandle JobBucket::GetTask(const Func& func)
{
	JobNode* node = sys->jobNodePool.New();
	jobNodesVec.emplace_back(node);
	node->Create<Func>(func, &sys->vectorPool, &sys->threadMtx);
	JobHandle retValue(node);
	return retValue;
}

template <typename Func>
void JobNode::Create(const Func& func, VectorPool* vectorPool, std::mutex* threadMtx)
{
	this->threadMtx = threadMtx;
	this->vectorPool = vectorPool;
	dependingEvent = vectorPool->New();
	using Storage = Storage<Func, 1>;
	if (sizeof(Storage) >= sizeof(FuncStorage))	//Create in heap
	{
		assert(false);
		ptr = new Func{
			std::move(func)
		};
		destructorFunc = [](void* currPtr)->void
		{
			Func* fc = (Func*)currPtr;
			delete fc;
		};
	}
	else
	{
		ptr = &stackArr;
		new (ptr)Func{
			std::move(func)
		};
		destructorFunc = [](void* currPtr)->void
		{
			Func* fc = (Func*)currPtr;
			fc->~Func();
		};
	}
	executeFunc = [](void* currPtr)->void
	{
		Func* fc = (Func*)currPtr;
		(*fc)();
	};
}