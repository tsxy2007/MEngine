#include "JobBucket.h"
ConcurrentPool<JobNode> JobBucket::jobNodePool(100);