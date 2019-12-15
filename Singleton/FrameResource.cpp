#include "FrameResource.h"
std::vector<std::unique_ptr<FrameResource>> FrameResource::mFrameResources;
Pool<ThreadCommand> FrameResource::threadCommandMemoryPool(20);
FrameResource* FrameResource::mCurrFrameResource = nullptr;
FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount)
{
	threadCommands.reserve(20);
	cameraCBs.reserve(50);
	objectCBs.reserve(50);
	afterFlushEvents.reserve(10);
}

void FrameResource::UpdateBeforeFrame(ID3D12Fence* mFence)
{
	if (Fence != 0 && mFence->GetCompletedValue() < Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
		for (int i = 0; i < afterFlushEvents.size(); ++i)
		{
			(*afterFlushEvents[i])();
			afterFlushEvents[i] = nullptr;
		}
		afterFlushEvents.clear();
	}
}
void FrameResource::UpdateAfterFrame(UINT64& currentFence, ID3D12CommandQueue* commandQueue, ID3D12Fence* mFence)
{
	// Advance the fence value to mark commands up to this fence point.
	Fence = ++currentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	commandQueue->Signal(mFence, currentFence);
}

void FrameResource::AddAfterFlushEvent(std::shared_ptr<std::function<void()>>& func)
{
	afterFlushEvents.push_back(func);
}


ThreadCommand* FrameResource::GetNewThreadCommand(ID3D12Device* device)
{
	if (threadCommands.size() <= 0)
	{
		return threadCommandMemoryPool.New(device);
	}
	else
	{
		ThreadCommand* result = threadCommands[threadCommands.size() - 1];
		threadCommands.erase(threadCommands.end() - 1);
		return result;
	}
}
void FrameResource::ReleaseThreadCommand(ThreadCommand* targetCmd)
{
	threadCommands.push_back(targetCmd);
}

FrameResource::~FrameResource()
{
	/*for (int i = 0; i < threadCommands.size(); ++i)
	{
		threadCommandMemoryPool.Delete(threadCommands[i]);
	}*/
}