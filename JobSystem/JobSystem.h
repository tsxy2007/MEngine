#pragma once
#include <mutex>
#include "JobBucket.h"
class JobBucket;
class JobThreadRunnable;
class JobSystem
{
	friend class JobThreadRunnable;
private:
	static void UpdateNewBucket();
	static int mThreadCount;
public:
	static void Initialize(int threadCount);
	static void ExecuteBucket(JobBucket** bucket, int bucketCount);
	static void ExecuteBucket(JobBucket* bucket, int bucketCount);
	static void Wait();
	static void Dispose();
};

