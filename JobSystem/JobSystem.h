#pragma once
#include "VectorPool.h"
#include "../Common/Pool.h"
#include "ConcurrentQueue.h"
class JobNode;
class JobBucket;
class JobThreadRunnable;

class JobSystem
{
	friend class JobBucket;
	friend class JobThreadRunnable;
	friend class JobNode;
private:

	std::mutex threadMtx;
	VectorPool vectorPool;
	void UpdateNewBucket();
	int mThreadCount;
	ConcurrentPool<JobNode> jobNodePool;
	ConcurrentQueue<JobNode*> executingNode;
	std::vector<std::thread*> allThreads;
	std::atomic<int> bucketMissionCount;
	int currentBucketPos;
	std::vector<JobBucket*> buckets;
	std::condition_variable cv;
	bool JobSystemInitialized;

	std::mutex mainThreadWaitMutex;
	std::condition_variable mainThreadWaitCV;
	bool mainThreadFinished;
public:
	JobSystem(int threadCount);
	void ExecuteBucket(JobBucket** bucket, int bucketCount);
	void ExecuteBucket(JobBucket* bucket, int bucketCount);
	void Wait();
	~JobSystem();
};

