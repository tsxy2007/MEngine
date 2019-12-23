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
RenderPipeline::RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) : renderPathComponents(3)
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

void RenderPipeline::RenderCamera(RenderPipelineData& renderData)
{
	std::vector <JobBucket>& bucketArray = buckets[bucketsFlag];
	bucketsFlag = !bucketsFlag;
	bucketArray.resize(renderData.allCameras->size());
	for (UINT camIndex = 0; camIndex < renderData.allCameras->size(); ++camIndex)
	{
		JobBucket& bucket = bucketArray[camIndex];
		Camera* cam = (*renderData.allCameras)[camIndex];
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
		data.device = renderData.device;
		data.backBuffer = renderData.backBufferResource;
		data.resource = renderData.resource;
		data.backBufferHandle = renderData.backBufferHandle;
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			component->threadCommand = InitThreadCommand(renderData.device, cam, renderData.resource, component);
			component->ExecuteTempRTCommand(renderData.device, &tempRTAllocator);
			
			JobHandle currentTask = component->RenderEvent(data, bucket, component->threadCommand);
			allPipelineTasks.insert_or_assign(component, currentTask);
		}
		//Build events dependency
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			ExecuteThreadCommand(renderData.resource->executableCommandList, cam, component->threadCommand);
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
	JobSystem::ExecuteBucket(bucketArray.data(), bucketArray.size());

	if (renderData.lastResource != nullptr)
	{
		//Final Execute
		if (renderData.executeLastFrame && renderData.lastResource->executableCommandList.size() > 0)
			renderData.commandQueue->ExecuteCommandLists(renderData.lastResource->executableCommandList.size(), renderData.lastResource->executableCommandList.data());
		//Finalize Frame
		renderData.lastResource->executableCommandList.clear();
		renderData.lastResource->UpdateAfterFrame(*renderData.fenceIndex, renderData.commandQueue, renderData.fence);
	}
	
	ThrowIfFailed(renderData.swap->Present(0, 0));
	tempRTAllocator.CumulateReleaseAfterFrame();
}