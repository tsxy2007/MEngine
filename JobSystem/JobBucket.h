#pragma once
#include <vector>
#include "JobHandle.h"
#include "../Common/Pool.h"
class JobSystem;
class JobThreadRunnable;
class JobNode;
class JobBucket
{
	friend class JobSystem;
	friend class JobNode;
	friend class JobHandle;
	friend class JobThreadRunnable;
private:
	std::vector<JobNode*> jobNodesVec;
	JobSystem* sys = nullptr;
public:
	JobBucket();
	void SetJobSystem(JobSystem* sys);
	template <typename Func>
	JobHandle GetTask(const Func& func);
};