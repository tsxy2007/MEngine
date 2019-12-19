#pragma once
#include "PipelineComponent.h"
#include "TempRTAllocator.h"
#include "../Common/d3dUtil.h"
#include "../taskflow/taskflow.hpp"
#include <unordered_map>
#include "../Common/MetaLib.h"
class FrameResource;
class Camera;
class RenderPipeline final
{
private:
	UINT initCount = 0;
	AlignedTuple<TestComponent> components; //TODO All Event Placement
	TempRTAllocator tempRTAllocator;
	std::unordered_map<PipelineComponent*, tf::Task> allPipelineTasks;
	std::unordered_map<std::string, PipelineComponent*> componentsLink;
	std::unordered_map<PipelineComponent*, std::vector<PipelineComponent*>*> dependMap;

	tf::Taskflow taskFlower;
	std::vector<std::vector<PipelineComponent*>> renderPathComponents;
	std::vector<ID3D12CommandList*> commandLists;
	template<typename T, typename ... Args>
	void Init(Args... args)
	{
		T* ptr = (T*)components[initCount];
		initCount++;
		componentsLink[std::string(typeid(T).name())] = ptr;
		new (ptr)T(args...);
	}
public:
	RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* directCommandList);
	//~RenderPipeline();
	void RenderCamera(ID3D12Device* device, ID3D12CommandQueue* commandQueue, FrameResource* resource, std::vector<Camera*>& allCameras, tf::Executor& executor);
};