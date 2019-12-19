#include "RenderPipeline.h"
#include "../Common/Camera.h"

RenderPipeline::RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* directCommandList)
{
	//TODO
	//Init All Events Here
	Init<TestComponent>();
	
	for (UINT i = 0; i < components.GetObjectCount(); ++i)
	{
		PipelineComponent* pcPtr = (PipelineComponent*)components[i];
		std::vector<std::string> dependNames = pcPtr->GetDependedEvent();
		std::vector<PipelineComponent*>* dependComponents = new std::vector<PipelineComponent*>;
		dependComponents->reserve(dependNames.size());
		for (UINT j = 0; j < dependNames.size(); ++j)
		{
			auto&& ite = componentsLink.find(dependNames[j]);
			if (ite != componentsLink.end())
			{
				dependComponents->emplace_back(ite->second);
			}
		}
		dependMap.insert_or_assign(pcPtr, std::move(dependComponents));
	}
}

void RenderPipeline::RenderCamera(ID3D12Device* device, ID3D12CommandQueue* commandQueue, FrameResource* resource, std::vector<Camera*>& allCameras, tf::Executor& executor)
{
	for (UINT camIndex = 0; camIndex < allCameras.size(); ++camIndex)
	{
		Camera* cam = allCameras[camIndex];
		std::vector<PipelineComponent*> waitingComponents = renderPathComponents[(UINT)cam->GetRenderingPath()];
		allPipelineTasks.clear();
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{ 
			PipelineComponent* component = waitingComponents[i];
			component->InitThreadCommand(device, cam, resource);
			std::vector<RenderTextureDescriptor>& descriptors = component->SendRenderTextureRequire();
			//Allocate Temporal Render Texture
			component->allTempRT.resize(descriptors.size());
			tempRTAllocator.GetRenderTextures(device, descriptors.data(), component->allTempRT.data(), component->allTempRT.size());
			allPipelineTasks.insert_or_assign(component, std::move(component->RenderEvent(device, taskFlower)));
		}
		//Build events dependency
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			component->ExecuteThreadCommand(commandLists, cam);//Add Command Lists to waiting vector
			auto&& ite = dependMap.find(component);
			if (ite != dependMap.end())
			{
				tf::Task& task = allPipelineTasks[component];
				for (UINT j = 0; j < ite->second->size(); ++j)
				{
					auto&& dependIte = allPipelineTasks.find((*ite->second)[j]);
					if (dependIte != allPipelineTasks.end())
					{
						if (!dependIte->second.empty()) dependIte->second.precede(task);
					}
				}
			}
		}
	}
	//Thread Sync, CPU Busy, Main Thread sleep
	executor.run(taskFlower).wait();
	//Final Execute
	commandQueue->ExecuteCommandLists(commandLists.size(), commandLists.data());
	//Finalize Frame
	commandLists.clear();
	taskFlower.clear();
	tempRTAllocator.CumulateReleaseAfterFrame();
}