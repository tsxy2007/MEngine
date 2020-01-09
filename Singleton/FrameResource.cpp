#include "../PipelineComponent/CommandBuffer.h"
#include "FrameResource.h"
#include "../Common/Camera.h"
std::vector<std::unique_ptr<FrameResource>> FrameResource::mFrameResources;
Pool<ThreadCommand> FrameResource::threadCommandMemoryPool(100);
Pool<FrameResource::FrameResCamera> FrameResource::perCameraDataMemPool(50);
std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> FrameResource::needClearResourcesAfterFlush;
FrameResource* FrameResource::mCurrFrameResource = nullptr;
CBufferPool FrameResource::cameraCBufferPool(sizeof(PassConstants), 256);
FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount)
{
	perCameraDatas.reserve(20);
	cameraCBs.reserve(50);
	objectCBs.reserve(50);
	needClearResources.reserve(10);
	needClearResourcesAfterFlush.reserve(10);
	commmonThreadCommand = new ThreadCommand(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
}

void FrameResource::UpdateBeforeFrame(ID3D12Fence** mFence, UINT fenceCount)
{
	if (Fence != 0)
	{
		for (UINT i = 0; i < fenceCount; ++i)
		{
			if (mFence[i]->GetCompletedValue() < Fence)
			{
				HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
				ThrowIfFailed(mFence[i]->SetEventOnCompletion(Fence, eventHandle));
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}
		}
		needClearResources.clear();
		needClearResourcesAfterFlush.clear();
	}
}
void FrameResource::UpdateAfterFrame(UINT64& currentFence, ID3D12CommandQueue** commandQueue, ID3D12Fence** mFence, UINT commandQueueCount)
{
	// Advance the fence value to mark commands up to this fence point.
	Fence = ++currentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	for(UINT i= 0; i < commandQueueCount; ++i)
		commandQueue[i]->Signal(mFence[i], currentFence);
}

void FrameResource::OnLoadCamera(Camera* targetCamera, ID3D12Device* device)
{
	perCameraDatas[targetCamera] = perCameraDataMemPool.New();
	ConstBufferElement constBuffer = cameraCBufferPool.Get(device);
	cameraCBs[targetCamera->GetInstanceID()] = constBuffer;
}
void FrameResource::OnUnloadCamera(Camera* targetCamera)
{
	FrameResCamera* data = perCameraDatas[targetCamera];
	for (int i = 0; i < data->threadCommands.size(); ++i)
	{
		threadCommandMemoryPool.Delete(data->threadCommands[i]);
	}
	perCameraDatas.erase(targetCamera);
	ConstBufferElement& constBuffer = cameraCBs[targetCamera->GetInstanceID()];
	cameraCBufferPool.Return(constBuffer);
	cameraCBs.erase(targetCamera->GetInstanceID());
	perCameraDataMemPool.Delete(data);
}

void FrameResource::ReleaseResourceAfterFlush(Microsoft::WRL::ComPtr<ID3D12Resource>& resources, FrameResource* resource)
{
	if (resource == nullptr)
		needClearResourcesAfterFlush.push_back(resources);
	else
		resource->needClearResources.push_back(resources);
}


ThreadCommand* FrameResource::GetNewThreadCommand(Camera* cam, ID3D12Device* device, D3D12_COMMAND_LIST_TYPE cmdListType)
{
	std::vector<ThreadCommand*>& threadCommands = perCameraDatas[cam]->threadCommands;
	if (threadCommands.size() <= 0)
	{
		return threadCommandMemoryPool.New(device, cmdListType);
	}
	else
	{
		ThreadCommand* result = threadCommands[threadCommands.size() - 1];
		threadCommands.erase(threadCommands.end() - 1);
		return result;
	}
}
void FrameResource::ReleaseThreadCommand(Camera* cam, ThreadCommand* targetCmd)
{
	std::vector<ThreadCommand*>& threadCommands = perCameraDatas[cam]->threadCommands;
	threadCommands.push_back(targetCmd);
}

FrameResource::~FrameResource()
{
	if (mCurrFrameResource == this)
		mCurrFrameResource = nullptr;
	for (auto ite = perFrameResource.begin(); ite != perFrameResource.end(); ++ite)
	{
		delete ite->second;
	}
	delete commmonThreadCommand;
}
