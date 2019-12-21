#include "RenderPipeline.h"
#include "../Common/Camera.h"
#include "../PipelineComponent/ThreadCommand.h"
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
RenderPipeline::RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* directCommandList) : renderPathComponents(3)
{
	//TODO
	//Init All Events Here
	Init<TestComponent>();
	
	//TODO
	//Init Path
	//renderPathComponents[0].push_back((PipelineComponent*)components[0]);
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
			else
			{
				throw "Non Exsis depended class!";
			}
		}
		dependMap.insert_or_assign(pcPtr, std::move(dependComponents));
	}
}

void RenderPipeline::RenderCamera(
	ID3D12Device* device,
	ID3D12CommandQueue* commandQueue,
	FrameResource* lastResource,
	FrameResource* resource,
	std::vector<Camera*>& allCameras,
	tf::Executor& executor,
	ID3D12Fence* fence,
	UINT64& fenceIndex)
{
	for (UINT camIndex = 0; camIndex < allCameras.size(); ++camIndex)
	{
		Camera* cam = allCameras[camIndex];
		std::vector<PipelineComponent*>& waitingComponents = renderPathComponents[(UINT)cam->GetRenderingPath()];
		allPipelineTasks.clear();
		renderTextureMarks.Clear();
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			std::vector<TemporalRTCommand>& descriptors = component->SendRenderTextureRequire();
			//Allocate Temporal Render Texture
			component->allTempRT.resize(descriptors.size());
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
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{ 
			PipelineComponent* component = waitingComponents[i];
			component->threadCommand = InitThreadCommand(device, cam, resource, component);
			component->ExecuteTempRTCommand(device, &tempRTAllocator);
			allPipelineTasks.insert_or_assign(component, std::move(component->RenderEvent(device, resource->taskFlow, component->threadCommand)));
		}
		//Build events dependency
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			ExecuteThreadCommand(resource->executableCommandList, cam, component->threadCommand);
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
	resource->waiter = executor.run(resource->taskFlow);
	if (lastResource != nullptr)
	{
		lastResource->waiter.wait();
		lastResource->taskFlow.clear();
		//Final Execute
		if (lastResource->executableCommandList.size() > 0)
			commandQueue->ExecuteCommandLists(lastResource->executableCommandList.size(), lastResource->executableCommandList.data());
		//Finalize Frame
		lastResource->executableCommandList.clear();
		lastResource->UpdateAfterFrame(fenceIndex, commandQueue, fence);
	}
	tempRTAllocator.CumulateReleaseAfterFrame();
}