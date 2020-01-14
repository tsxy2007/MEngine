#include "RenderPipeline.h"
#include "PipelineComponent.h"
#include "../Common/Camera.h"
#include "../PipelineComponent/ThreadCommand.h"
#include "PrepareComponent.h"
#include "GBufferComponent.h"
#include "../LogicComponent/World.h"
#include "SkyboxComponent.h"
#include "PostProcessingComponent.h"
#include "../RenderComponent/RenderCommand.h"
//ThreadCommand* threadCommand;
RenderPipeline* RenderPipeline::current(nullptr);
std::unordered_map<std::string, PipelineComponent*> RenderPipeline::componentsLink;
ThreadCommand* InitThreadCommand(ID3D12Device* device, Camera* cam, FrameResource* resource, PipelineComponent* comp)
{
	switch (comp->GetCommandListType())
	{
	case CommandListType_Graphics:
		return resource->GetNewThreadCommand(cam, device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		break;
	case CommandListType_Compute:
		return resource->GetNewThreadCommand(cam, device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	default:
		return nullptr;
	}
}
void ExecuteThreadCommand(Camera* cam, ThreadCommand* command, FrameResource* resource)
{
	if (command != nullptr) {
		resource->ReleaseThreadCommand(cam, command);
	}
}

RenderPipeline::RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) : renderPathComponents(3)
{
	current = this;
	//Init All Events Here
	Init<PrepareComponent>();
	Init<GBufferComponent>();
	Init<SkyboxComponent>();
	Init<PostProcessingComponent>();

	for (UINT i = 0, size = components.size(); i < size; ++i)
	{
		PipelineComponent* comp = components[i];
		comp->Initialize(device, commandList);
		for (auto ite = comp->gpuDepending.begin(); ite != comp->gpuDepending.end(); ++ite)
		{
			(*ite)->CreateFence(device);
		}
	}
	//Init Path
	for (auto i = components.begin(); i != components.end(); ++i)
	{
		renderPathComponents[0].push_back(*i);
	}
}

void RenderPipeline::RenderCamera(RenderPipelineData& renderData, JobSystem* jobSys)
{
	CommandBuffer* currentCommandBuffer = renderData.resource->commandBuffer.get();
	std::vector <JobBucket*>& bucketArray = buckets[bucketsFlag];
	bucketsFlag = !bucketsFlag;
	UINT camSize = max(renderData.allCameras->size(), 1);
	for (int i = bucketArray.size(); i <= camSize; ++i)
	{
		bucketArray.push_back(jobSys->GetJobBucket());
	}
	PipelineComponent::EventData data;
	data.device = renderData.device;
	data.resource = renderData.resource;
	data.world = renderData.world;
	UINT frameNum = *renderData.fenceIndex + 2;

	ThreadCommand* commandList = renderData.resource->commmonThreadCommand;
	bucketArray[0]->GetTask([=]()->void
	{
		commandList->ResetCommand();
		while (RenderCommand::ExecuteCommand(
			data.device, commandList->GetCmdList(), renderData.resource))
		{
		}
		commandList->CloseCommand();
	});
	currentCommandBuffer->ExecuteGraphicsCommandList(commandList->GetCmdList());
	for (UINT camIndex = 0; camIndex < renderData.allCameras->size(); ++camIndex)
	{
		JobBucket* bucket = bucketArray[camIndex];
		Camera* cam = (*renderData.allCameras)[camIndex];

		if (cam->renderTarget)
		{
			data.width = cam->renderTarget->GetWidth();
			data.height = cam->renderTarget->GetHeight();
			data.isBackBufferForPresent = false;
			data.backBufferHandle = cam->renderTarget->GetColorDescriptor(0);
			data.backBuffer = cam->renderTarget->GetColorResource();
		}
		else
		{
			data.width = data.world->windowWidth;
			data.height = data.world->windowHeight;
			data.isBackBufferForPresent = true;
			data.backBuffer = renderData.backBufferResource;
			data.backBufferHandle = renderData.backBufferHandle;
		}
		data.camera = cam;
		std::vector<PipelineComponent*>& waitingComponents = renderPathComponents[(UINT)cam->GetRenderingPath()];
		renderTextureMarks.Clear();
		//Read Events' Markers
		for (UINT i = 0, size = waitingComponents.size(); i < size; ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			std::vector<TemporalResourceCommand>& descriptors = component->SendRenderTextureRequire(data);
			//Allocate Temporal Render Texture
			component->allTempResource.resize(descriptors.size());
			for (UINT j = 0, descriptorSize = descriptors.size(); j < descriptorSize; ++j)
			{
				TemporalResourceCommand& command = descriptors[j];
				if (command.type == TemporalResourceCommand::CommandType_Create_RenderTexture ||
					command.type == TemporalResourceCommand::CommandType_Create_StructuredBuffer)
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
					//command.tempResState
					component->requiredRTs.push_back({
						j,
						command.uID });
				}
			}

		}
		//Mark Resources throw Markers
		for (UINT i = 0, size = renderTextureMarks.values.size(); i < size; ++i)
		{
			RenderTextureMark& mark = renderTextureMarks.values[i].value;
			waitingComponents[mark.startComponent]->loadRTCommands.push_back({ mark.id, mark.rtIndex, mark.desc });
			waitingComponents[mark.endComponent]->unLoadRTCommands.push_back(mark.id);
		}

		PipelineComponent::bucket = bucket;

		//Execute Events
		for (auto componentIte = waitingComponents.begin(); componentIte != waitingComponents.end(); ++componentIte)
		{
			PipelineComponent* component = *componentIte;
			component->threadCommand = InitThreadCommand(renderData.device, cam, renderData.resource, component);
			component->ExecuteTempRTCommand(renderData.device, &tempRTAllocator);
			component->ClearHandles();
			component->RenderEvent(data, component->threadCommand);
			switch (component->GetCommandListType())
			{
			case CommandListType_Graphics:
				for (auto ite = component->gpuDepending.begin(); ite != component->gpuDepending.end(); ++ite)
				{
					currentCommandBuffer->WaitForGraphics((*ite)->fence.Get(), frameNum);
				}
				currentCommandBuffer->ExecuteGraphicsCommandList(component->threadCommand->GetCmdList());
				if (component->dependingComponentCount > 0)
				{
					currentCommandBuffer->SignalToGraphics(component->fence.Get(), frameNum);
				}
				break;
			case CommandListType_Compute:
				for (auto ite = component->gpuDepending.begin(); ite != component->gpuDepending.end(); ++ite)
				{
					currentCommandBuffer->WaitForCompute((*ite)->fence.Get(), frameNum);
				}
				currentCommandBuffer->ExecuteComputeCommandList(component->threadCommand->GetCmdList());
				if (component->dependingComponentCount > 0)
				{
					currentCommandBuffer->SignalToCompute(component->fence.Get(), frameNum);
				}
				break;
			}
		}

		for (UINT i = 0, size = waitingComponents.size(); i < size; ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			component->MarkHandles();
			ExecuteThreadCommand(cam, component->threadCommand, renderData.resource);
		}
	}
	FrameResource::mCurrFrameResource->UpdateBeforeFrame(renderData.fence, renderData.fenceCount);//Flush CommandQueue
	jobSys->ExecuteBucket(bucketArray.data(), camSize);					//Execute Tasks

	if (renderData.lastResource != nullptr)				//Run Last frame's commandqueue
	{
		ID3D12CommandQueue* queues[3] =
		{
			renderData.lastResource->commandBuffer->GetGraphicsQueue(),
			renderData.lastResource->commandBuffer->GetComputeQueue(),
			renderData.lastResource->commandBuffer->GetAsyncQueue()
		};
		//Final Execute
		if (renderData.executeLastFrame)
		{
			renderData.lastResource->commandBuffer->Submit();
		}
		//Finalize Frame

		renderData.lastResource->UpdateAfterFrame(*renderData.fenceIndex, queues, renderData.fence, renderData.fenceCount);//Signal CommandQueue
		renderData.lastResource->commandBuffer->Clear();
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