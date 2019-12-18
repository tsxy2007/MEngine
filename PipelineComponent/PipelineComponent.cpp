#include "PipelineComponent.h"
#include "../PipelineComponent/ThreadCommand.h"
#include "../Singleton/FrameResource.h"
RenderTexture* PipelineComponent::GetTempRT(UINT index)
{
	return allTempRT[index];
}

void PipelineComponent::AddAllTempRT(RenderTexture** rts, UINT length)
{
	allTempRT.clear();
	for (int i = 0; i < length; ++i)
	{
		allTempRT.push_back(rts[i]);
	}
}

void PipelineComponent::InitThreadCommand(ID3D12Device* device)
{
	threadCommand = FrameResource::mCurrFrameResource->GetNewThreadCommand(device);
}
void PipelineComponent::ExecuteThreadCommand(std::vector<ID3D12CommandList*>& executableCommands)
{
	if (threadCommand == nullptr) return;
	executableCommands.push_back(threadCommand->GetCmdList());
	FrameResource::mCurrFrameResource->ReleaseThreadCommand(threadCommand);
	threadCommand = nullptr;
}