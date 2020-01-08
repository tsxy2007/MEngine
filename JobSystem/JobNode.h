#pragma once
#include <vector>
#include <atomic>
#include <condition_variable>
#include "ConcurrentQueue.h"
#include "../Common/Pool.h"
#include <DirectXMath.h>
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
	struct FuncStorage
	{
		alignas(__m128) char arr[sizeof(__m128) * 16];
	};
	class VectorPool
	{
		std::mutex mtx;
		Pool<std::vector<JobNode*>> pool;
		std::vector< std::vector<JobNode*>*> cache;
	public:
		VectorPool() : pool(50)
		{
		}
		~VectorPool();
		std::vector<JobNode*>* Get();
		void Return(std::vector<JobNode*>* node);
	};
	static std::mutex threadMtx;
	static VectorPool vecPool;
	std::atomic<unsigned int> targetDepending = 0;
	std::vector<JobNode*>* dependingEvent;
	FuncStorage stackArr;
	void* ptr = nullptr;
	void(*destructorFunc)(void*) = nullptr;
	void(*executeFunc)(void*);
	template <typename Func>
	void Create(Func&& func)
	{
		dependingEvent = vecPool.Get();
		using Storage = std::aligned_storage_t<sizeof(Func), alignof(Func)>;
		if (sizeof(Storage) >= sizeof(FuncStorage))	//Create in heap
		{
			assert(false);
			ptr = new Func(std::forward<Func>(func));
			destructorFunc = [](void* currPtr)->void
			{
				Func* fc = (Func*)currPtr;
				delete fc;
			};
		}
		else
		{
			ptr = &stackArr;
			new (ptr)Func(std::forward<Func>(func));
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
	JobNode* Execute(ConcurrentQueue<JobNode*>& taskList, std::condition_variable& cv);
	void Precede(JobNode* depending);
public:

	~JobNode();
};