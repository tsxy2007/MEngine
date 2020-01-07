#include "JobNode.h"
std::mutex JobNode::threadMtx;
JobNode::VectorPool JobNode::vecPool;
JobNode::~JobNode()
{
	if (destructorFunc != nullptr)
		destructorFunc(ptr);
	dependingEvent->clear();
	vecPool.Return(dependingEvent);
}

JobNode::VectorPool::~VectorPool()
{
	for (auto ite = cache.begin(); ite != cache.end(); ++ite)
	{
		pool.Delete(*ite);
	}
}
std::vector<JobNode*>*JobNode::VectorPool::Get()
{
	if (cache.empty())
	{
		auto v = pool.New();
		v->reserve(20);
		return v;
	}
	auto&& ite = cache.end() - 1;
	std::vector<JobNode*>* ptr = *ite;
	cache.erase(ite);
	return ptr;
}
void JobNode::VectorPool::Return(std::vector<JobNode*>* node)
{
	std::lock_guard<std::mutex> lck(mtx);
	cache.push_back(node);
}
JobNode* JobNode::Execute(ConcurrentQueue<JobNode*>& taskList, std::condition_variable& cv)
{
	executeFunc(ptr);
	std::vector<JobNode*>::iterator ite = dependingEvent->begin();
	JobNode* nextNode = nullptr;
	while(ite != dependingEvent->end())
	{
		JobNode* node = *ite;
		unsigned int dependingCount = --node->targetDepending;
		if (dependingCount == 0)
		{
			nextNode = node;
			++ite;
			break;
		}
		++ite;
	}
	for (; ite != dependingEvent->end(); ++ite)
	{
		JobNode* node = *ite;
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
	return nextNode;
}

void JobNode::Precede(JobNode* depending)
{
	depending->targetDepending++;
	dependingEvent->emplace_back(depending);
}