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
	AlignedTuple<int> components;//TODO
	TempRTAllocator tempRTAllocator;
	std::unordered_map<std::string, PipelineComponent*> componentPtrs;
	std::vector<std::vector<PipelineComponent*>> renderPathComponents;
	std::vector<ID3D12CommandList*> commandLists;
	tf::Taskflow taskFlow;
public:
	RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* directCommandList);
	~RenderPipeline();
	void RenderCamera(ID3D12Device* device, ID3D12CommandQueue* commandQueue, FrameResource* resource, std::vector<Camera*>& allCameras, tf::Executor& executor);
};