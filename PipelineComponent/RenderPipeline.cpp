#include "RenderPipeline.h"
#include "../Common/Camera.h"
#include "../PipelineComponent/ThreadCommand.h"
#include "PrepareComponent.h"
#include "GBufferComponent.h"
#include "../LogicComponent/World.h"
#include "SkyboxComponent.h"
//ThreadCommand* threadCommand;
RenderPipeline* RenderPipeline::current(nullptr);
std::unordered_map<std::string, PipelineComponent*> RenderPipeline::componentsLink;
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

PipelineComponent* RenderPipeline::GetComponent(const char* typeName)
{
	std::string str(typeName);
	auto&& ite = componentsLink.find(str);
	if (ite != componentsLink.end())
		return ite->second;
	else return nullptr;
}

RenderPipeline::RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) : renderPathComponents(3)
{
	current = this;
	//TODO
	//Init All Events Here
	Init<PrepareComponent>();
	Init<GBufferComponent>();
	Init<SkyboxComponent>();

	for (UINT i = 0, size = components.size(); i < size; ++i)
	{
		components[i]->Initialize(device, commandList);
	}
	//TODO
	//Init Path
	renderPathComponents[0].push_back(components[0]);
	renderPathComponents[0].push_back(components[1]);
	renderPathComponents[0].push_back(components[2]);
}

void RenderPipeline::RenderCamera(RenderPipelineData& renderData)
{
	std::vector <JobBucket>& bucketArray = buckets[bucketsFlag];
	bucketsFlag = !bucketsFlag;
	bucketArray.resize(renderData.allCameras->size() + 1);
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
		for (UINT i = 0, size = waitingComponents.size(); i < size; ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			std::vector<TemporalRTCommand>& descriptors = component->SendRenderTextureRequire(data);
			//Allocate Temporal Render Texture
			component->allTempRT.resize(descriptors.size());
			for (UINT j = 0, descriptorSize = descriptors.size(); j < descriptorSize; ++j)
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
					component->requiredRTs.emplace_back(j, command.uID);
				}
			}

		}

		for (UINT i = 0, size = renderTextureMarks.values.size(); i < size; ++i)
		{
			RenderTextureMark& mark = renderTextureMarks.values[i].value;
			waitingComponents[mark.startComponent]->loadRTCommands.emplace_back(mark.id, mark.rtIndex, mark.desc);
			waitingComponents[mark.endComponent]->unLoadRTCommands.emplace_back(mark.id);
		}


		for (UINT i = 0, size = waitingComponents.size(); i < size; ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			component->threadCommand = InitThreadCommand(renderData.device, cam, renderData.resource, component);
			component->ExecuteTempRTCommand(renderData.device, &tempRTAllocator);
			component->RenderEvent(data, bucket, component->threadCommand);
		}
		for (UINT i = 0, size = waitingComponents.size(); i < size; ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			ExecuteThreadCommand(renderData.resource->executableCommandList, cam, component->threadCommand);
		}
	}
	ThreadCommand* commandList = renderData.resource->commmonThreadCommand;
	bucketArray[renderData.allCameras->size()].GetTask([=]()->void
	{
		commandList->ResetCommand();
		commandList->CloseCommand();
	});
	renderData.resource->executableCommandList.emplace_back(commandList->GetCmdList());
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
		delete components[i];
	}

}

RenderPipeline* RenderPipeline::GetInstance(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	if (current == nullptr)
	{
		current = new RenderPipeline(device, commandList);
	}
	return current;
}

void RenderPipeline::DestroyInstance()
{
	if (current != nullptr)
	{
		auto ptr = current;
		current = nullptr;
		delete ptr;
	}
}