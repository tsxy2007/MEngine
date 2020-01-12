#include "JobBucket.h"
#include "JobNode.h"
JobBucket::JobBucket()
{
	jobNodesVec.reserve(20);
}
void JobBucket::SetJobSystem(JobSystem* sys)
{
	this->sys = sys;
}