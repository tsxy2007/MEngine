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
	struct RenderTextureMark
	{
		UINT id;
		UINT rtIndex;
		RenderTextureDescriptor desc;
		UINT startComponent;
		UINT endComponent;
	};
	UINT initCount = 0;
	std::vector<PipelineComponent*> components;
	TempRTAllocator tempRTAllocator;
	std::unordered_map<PipelineComponent*, tf::Task> allPipelineTasks;
	std::unordered_map<std::string, PipelineComponent*> componentsLink;
	std::unordered_map<PipelineComponent*, std::vector<PipelineComponent*>*> dependMap;
	std::vector<std::vector<PipelineComponent*>> renderPathComponents;
	Dictionary<UINT, RenderTextureMark> renderTextureMarks;
	std::future<void> waiter;
	bool taskFlowFlag = false;
	template<typename T, typename ... Args>
	void Init(Args... args)
	{
		T* ptr = new T(args...);
		components.emplace_back(ptr);
		componentsLink.insert_or_assign(typeid(T).name(), ptr);
	}
public:
	RenderPipeline(ID3D12Device* device);
	//~RenderPipeline();
	void RenderCamera(
		ID3D12Device* device,
		ID3D12Resource* backBufferResource,
		D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle,
		ID3D12CommandQueue* commandQueue, 
		FrameResource* lastResource,
		FrameResource* resource,
		std::vector<Camera*>& allCameras, 
		tf::Executor& executor,
		ID3D12Fence* fence,
		UINT64& fenceIndex,
		bool executeLastFrame,
		IDXGISwapChain* swap);
};