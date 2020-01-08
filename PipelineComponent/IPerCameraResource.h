#pragma once
#include "../Common/d3dUtil.h"
#include <mutex>
class Camera;
class PipelineComponent;
class FrameResource;
class IPipelineResource
{
public:
	virtual ~IPipelineResource() {}
};

class PipelineResourceManager;
class PipelineResourceContainer;
class PipelineResource
{
	friend class PipelineResourceContainer;
	friend class PipelineResourceManager;
private:
	PipelineResourceManager* manager;
	UINT target;
	PipelineResource(PipelineResourceManager* manager, UINT target) : manager(manager), target(target) {}
public:
	PipelineResource() {}
	void Dispose();
	PipelineResource(const PipelineResource& another);
	PipelineResource& operator=(const PipelineResource& another);
	IPipelineResource* GetResource();
};
class PipelineResourceManager
{
	friend class PipelineResourceContainer;
private:
	friend class PipelineResource;
	std::vector<PipelineResource> disposeList;
	std::vector<IPipelineResource*> resource;
	std::vector<UINT> usedIndices;
	std::mutex mtx;
	UINT InitResource(IPipelineResource* targetRes);
	void DisposeResource(PipelineResource&);
	void AddDisposeCommand(PipelineResource& res);
public:
	PipelineResourceManager(UINT capacity);
	void DisposeAllInList();
};

class PipelineResourceContainer
{
private:
	std::unordered_map<PipelineResourceManager*, std::unordered_map<void*, PipelineResource>> allResource;
public:
	PipelineResourceContainer();
	template<typename Func>
	IPipelineResource* GetResource(PipelineResourceManager* manager, void* targetPtr, Func&& func)
	{
		auto&& ite = allResource.find(manager);
		std::unordered_map<void*, PipelineResource>* map = nullptr;
		if (ite != allResource.end())
			map = &ite->second;
		else
		{
			allResource.insert_or_assign(manager, std::unordered_map<void*, PipelineResource>());
			map = &allResource[manager];
		}
		if (map->empty())
			map->reserve(20);
		auto&& resultIte = map->find(targetPtr);
		if (resultIte == map->end())
		{
			IPipelineResource* pipePtr = func();
			UINT target = manager->InitResource(pipePtr);
			PipelineResource result(manager, target);
			map->insert_or_assign(targetPtr, result);
			return result.GetResource();
		}
		return resultIte->second.GetResource();
	}
	template<typename Func>
	IPipelineResource* GetResource(PipelineResourceManager* manager, void* targetPtr, Func& func)
	{
		return GetResource<Func>(manager, targetPtr, std::forward<Func>(func));
	}
	~PipelineResourceContainer();
	void DisposeResource(PipelineResourceManager* manager, void* targetPtr);
};
