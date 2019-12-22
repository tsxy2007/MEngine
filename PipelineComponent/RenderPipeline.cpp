#include "RenderPipeline.h"
#include "../Common/Camera.h"
#include "../PipelineComponent/ThreadCommand.h"
#include "PrepareComponent.h"
#include "GBufferComponent.h"
//ThreadCommand* threadCommand;

ThreadCommand* InitThreadCommand(ID3D12Device* device, Camera* cam, FrameResource* resource, PipelineComponent* comp)
{
	if (comp->NeedCommandList())
		return resource->GetNewThreadCommand(cam, device);
	else return nullptr;
}
void ExecuteThreadCommand(std::vector<ID3D12CommandList*>& executableCommands, Camera* cam, ThreadCommand* command)
{
	if (command != nullptr) {
		executableCommands.emplace_back(command->GetCmdList());
		FrameResource::mCurrFrameResource->ReleaseThreadCommand(cam, command);
	}

}
RenderPipeline::RenderPipeline(ID3D12Device* device) : renderPathComponents(3)
{
	
	//TODO
	//Init All Events Here
	Init<PrepareComponent>();
	Init<GBufferComponent>();

	//TODO
	//Init Path
	renderPathComponents[0].push_back(components[0]);
	renderPathComponents[0].push_back(components[1]);
	for (UINT i = 0; i < components.size(); ++i)
	{
		PipelineComponent* pcPtr = components[i];
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
			else
			{
				throw "Non Exist depended class!";
			}
		}
		dependMap.insert_or_assign(pcPtr, std::move(dependComponents));
	}
}

void RenderPipeline::RenderCamera(
	ID3D12Device* device,
	ID3D12Resource* backBufferResource,
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle,
	ID3D12CommandQueue* commandQueue,
	FrameResource* lastResource,
	FrameResource* resource,
	std::vector<Camera*>& allCameras,
	ID3D12Fence* fence,
	UINT64& fenceIndex,
	bool lastFrame,
	IDXGISwapChain* swapChain)
{
	std::vector <JobBucket>& bucketArray = buckets[bucketsFlag];
	bucketsFlag = !bucketsFlag;
	bucketArray.resize(allCameras.size());
	for (UINT camIndex = 0; camIndex < allCameras.size(); ++camIndex)
	{
		JobBucket& bucket = bucketArray[camIndex];
		Camera* cam = allCameras[camIndex];
		std::vector<PipelineComponent*>& waitingComponents = renderPathComponents[(UINT)cam->GetRenderingPath()];
		renderTextureMarks.Clear();
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			std::vector<TemporalRTCommand>& descriptors = component->SendRenderTextureRequire();
			//Allocate Temporal Render Texture
			for (UINT j = 0; j < descriptors.size(); ++j)
			{
				TemporalRTCommand& command = descriptors[j];
				if (command.type == TemporalRTCommand::Create)
				{
					//Alread contained
					if (tempRTAllocator.GetUsingData(command.uID) != nullptr)
					{
						throw "Alread Created Render Texture!";
					}
					RenderTextureMark mark = { command.uID, j, command.descriptor, i, i };
					renderTextureMarks.Add(command.uID, mark);
				}
				else
				{
					RenderTextureMark* markPtr = renderTextureMarks[command.uID];
					if (markPtr == nullptr)
					{
						throw "No Such Render Texture!";
					}
					markPtr->endComponent = i;
				}
			}

		}

		for (UINT i = 0; i < renderTextureMarks.values.size(); ++i)
		{
			RenderTextureMark& mark = renderTextureMarks.values[i].value;
			waitingComponents[mark.startComponent]->loadRTCommands.push_back({ mark.id, mark.rtIndex, mark.desc });
			waitingComponents[mark.endComponent]->unLoadRTCommands.emplace_back(mark.id);
		}

		PipelineComponent::EventData data;
		data.camera = cam;
		data.device = device;
		data.backBuffer = backBufferResource;
		data.resource = resource;
		data.backBufferHandle = backBufferHandle;
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			component->threadCommand = InitThreadCommand(device, cam, resource, component);
			component->ExecuteTempRTCommand(device, &tempRTAllocator);
			
			JobHandle currentTask = component->RenderEvent(data, bucket, component->threadCommand);
			allPipelineTasks.insert_or_assign(component, currentTask);
		}
		//Build events dependency
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			ExecuteThreadCommand(resource->executableCommandList, cam, component->threadCommand);
			auto&& ite = dependMap.find(component);
			if (ite != dependMap.end())
			{
				JobHandle& task = allPipelineTasks[component];
				for (UINT j = 0; j < ite->second->size(); ++j)
				{
					auto&& dependIte = allPipelineTasks.find((*ite->second)[j]);
					if (dependIte != allPipelineTasks.end())
					{
						dependIte->second.Precede(task);
					}
				}
			}
		}
	}
	//Sleep(1);
	JobSystem::ExecuteBucket(bucketArray.data(), bucketArray.size());

	if (lastResource != nullptr)
	{
		//Final Execute
		if (lastFrame && lastResource->executableCommandList.size() > 0)
			commandQueue->ExecuteCommandLists(lastResource->executableCommandList.size(), lastResource->executableCommandList.data());
		//Finalize Frame
		lastResource->executableCommandList.clear();
		lastResource->UpdateAfterFrame(fenceIndex, commandQueue, fence);
	}
	
	ThrowIfFailed(swapChain->Present(0, 0));

	tempRTAllocator.CumulateReleaseAfterFrame();
	
	
}