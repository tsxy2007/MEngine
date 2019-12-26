#include "RenderPipeline.h"
#include "../Common/Camera.h"
#include "../PipelineComponent/ThreadCommand.h"
#include "PrepareComponent.h"
#include "GBufferComponent.h"
#include "../LogicComponent/World.h"
//ThreadCommand* threadCommand;
RenderPipeline* RenderPipeline::current(nullptr);
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
	current = this;
	//TODO
	//Init All Events Here
	Init<PrepareComponent>();
	Init<GBufferComponent>();


	for (UINT i = 0; i < components.size(); ++i)
	{
		components[i]->Initialize();
	}
	//TODO
	//Init Path
	renderPathComponents[0].push_back(components[0]);
	renderPathComponents[0].push_back(components[1]);
}

void RenderPipeline::RenderCamera(RenderPipelineData& renderData)
{
	std::vector <JobBucket>& bucketArray = buckets[bucketsFlag];
	bucketsFlag = !bucketsFlag;
	bucketArray.resize(renderData.allCameras->size());
	PipelineComponent::EventData data;
	data.device = renderData.device;
	data.backBuffer = renderData.backBufferResource;
	data.resource = renderData.resource;
	data.backBufferHandle = renderData.backBufferHandle;
	data.world = renderData.world;
	for (UINT camIndex = 0; camIndex < renderData.allCameras->size(); ++camIndex)
	{
		JobBucket& bucket = bucketArray[camIndex];
		Camera* cam = (*renderData.allCameras)[camIndex];
		data.camera = cam;
		std::vector<PipelineComponent*>& waitingComponents = renderPathComponents[(UINT)cam->GetRenderingPath()];
		renderTextureMarks.Clear();
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			std::vector<TemporalRTCommand>& descriptors = component->SendRenderTextureRequire(data);
			//Allocate Temporal Render Texture
			for (UINT j = 0; j < descriptors.size(); ++j)
			{
				TemporalRTCommand& command = descriptors[j];
				if (command.type == TemporalRTCommand::Create)
				{
					//Alread contained
					if (tempRTAllocator.Contains(command.uID))
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
			component->threadCommand = InitThreadCommand(renderData.device, cam, renderData.resource, component);
			component->ExecuteTempRTCommand(renderData.device, &tempRTAllocator);
			component->RenderEvent(data, bucket, component->threadCommand);
		}
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			ExecuteThreadCommand(renderData.resource->executableCommandList, cam, component->threadCommand);
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

RenderPipeline::~RenderPipeline()
{
	for (UINT i = 0; i < components.size(); ++i)
	{
		components[i]->Dispose();
	}
}