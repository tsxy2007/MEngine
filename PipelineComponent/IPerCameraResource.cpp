#include "IPerCameraResource.h"

PipelineResource::PipelineResource(const PipelineResource& another)
{
	memcpy(this, &another, sizeof(PipelineResource));
}
PipelineResource& PipelineResource::operator=(const PipelineResource& another)
{
	memcpy(this, &another, sizeof(PipelineResource));
	return *this;
}
PipelineResourceManager::PipelineResourceManager(UINT capacity)
{
	usedIndices.reserve(capacity);
	resource.reserve(capacity);
	disposeList.reserve(capacity);
}
void PipelineResourceManager::DisposeAllInList()
{
	for (auto ite = disposeList.begin(); ite != disposeList.end(); ++ite)
	{
		DisposeResource(*ite);
	}
	disposeList.clear();
}
PipelineResourceContainer::PipelineResourceContainer()
{
	allResource.reserve(20);
}
UINT PipelineResourceManager::InitResource(IPipelineResource* targetRes)
{
	std::lock_guard<std::mutex> lck(mtx);
	if (usedIndices.empty())
	{
		resource.push_back(targetRes);
		return resource.size() - 1;
	}
	else
	{
		auto&& ite = usedIndices.end() - 1;
		UINT result = *ite;
		resource[*ite] = targetRes;
		usedIndices.erase(ite);
		return result;
	}
}

void PipelineResourceManager::DisposeResource(PipelineResource& res)
{
	std::lock_guard<std::mutex> lck(mtx);
	delete resource[res.target];
	usedIndices.push_back(res.target);
}

IPipelineResource* PipelineResource::GetResource()
{
	return manager->resource[target];
}
void PipelineResource::Dispose()
{
	if (manager != nullptr)
	{
		manager->DisposeResource(*this);
	}
}
void PipelineResourceManager::AddDisposeCommand(PipelineResource& res)
{
	std::lock_guard<std::mutex> lck(mtx);
	disposeList.push_back(res);
}

void PipelineResourceContainer::DisposeResource(PipelineResourceManager* manager, void* targetPtr)
{
	auto&& ite = allResource.find(manager);
	std::unordered_map<void*, PipelineResource>* map = nullptr;
	if (ite != allResource.end())
		map = &ite->second;
	else return;
	auto&& resultIte = map->find(targetPtr);
	if (resultIte == map->end()) return;
	manager->AddDisposeCommand(resultIte->second);
	map->erase(resultIte);
}

PipelineResourceContainer::~PipelineResourceContainer()
{
	for (auto ite = allResource.begin(); ite != allResource.end(); ++ite)
	{
		for (auto i = ite->second.begin(); i != ite->second.end(); ++i)
			i->second.Dispose();
	}
}