#include "JobNode.h"
std::mutex JobNode::threadMtx;
JobNode::~JobNode()
{
	if (destructorFunc != nullptr)
		destructorFunc(ptr);
}
void JobNode::Execute(ConcurrentQueue<JobNode*>& taskList, std::condition_variable& cv)
{
	executeFunc(ptr);
	for (size_t i = 0; i < dependingEvent.size(); ++i)
	{
		JobNode* node = dependingEvent[i];
		unsigned int dependingCount = --node->targetDepending;
		if (dependingCount == 0)
		{
			taskList.Push(node);
			{
				std::lock_guard<std::mutex> lck(threadMtx);
				cv.notify_one();
			}
		}
	}

}

void JobNode::Precede(JobNode* depending)
{
	depending->targetDepending++;
	dependingEvent.emplace_back(depending);
}