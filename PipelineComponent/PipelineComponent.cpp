#include "PipelineComponent.h"
#include "../PipelineComponent/ThreadCommand.h"
#include "../Singleton/FrameResource.h"
RenderTexture* PipelineComponent::GetTempRT(UINT index)
{
	return allTempRT[index];
}

void PipelineComponent::InitThreadCommand(ID3D12Device* device, Camera* cam, FrameResource* resource)
{
	if (NeedCommandList() && threadCommand == nullptr)
		threadCommand = resource->GetNewThreadCommand(cam, device);
}
void PipelineComponent::ExecuteThreadCommand(std::vector<ID3D12CommandList*>& executableCommands, Camera* cam)
{
	if (threadCommand == nullptr) return;
	executableCommands.emplace_back(threadCommand->GetCmdList());
	FrameResource::mCurrFrameResource->ReleaseThreadCommand(cam, threadCommand);
	threadCommand = nullptr;
}