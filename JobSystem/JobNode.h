#pragma once
#include <vector>
#include <atomic>
#include <condition_variable>
#include "ConcurrentQueue.h"
class JobHandle;
class JobThreadRunnable;
class JobBucket;
class JobNode
{
	friend class JobBucket;
	friend class JobSystem;
	friend class JobHandle;
	friend class JobThreadRunnable;
private:
	static std::mutex threadMtx;
	std::atomic<unsigned int> targetDepending = 0;
	std::vector<JobNode*> dependingEvent;
	char stackArr[64];
	void* ptr = nullptr;
	void(*destructorFunc)(void*) = nullptr;
	void(*executeFunc)(void*);
	template <typename Func>
	void Create(Func&& func)
	{
		using Storage = std::aligned_storage_t<sizeof(Func), alignof(Func)>;
		if (sizeof(Storage) >= 64)	//Create in heap
		{
			ptr = new Func(std::move(func));
			destructorFunc = [](void* currPtr)->void
			{
				Func* fc = (Func*)currPtr;
				delete fc;
			};
		}
		else
		{
			ptr = stackArr;
			new (ptr)Func(std::move(func));
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
	void Execute(ConcurrentQueue<JobNode*>& taskList, std::condition_variable& cv);
	void Precede(JobNode* depending);
public:

	~JobNode();
};