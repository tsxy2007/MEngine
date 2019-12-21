#include "PipelineComponent.h"
#include "../Singleton/FrameResource.h"
#include "TempRTAllocator.h"
std::mutex PipelineComponent::mtx;
RenderTexture* PipelineComponent::GetTempRT(UINT index)
{
	return allTempRT[index];
}

void PipelineComponent::ExecuteTempRTCommand(ID3D12Device* device, TempRTAllocator* allocator)
{
	for (UINT i = 0; i < loadRTCommands.size(); ++i)
	{
		LoadTempRTCommand& cmd = loadRTCommands[i];
		allTempRT[cmd.index] = allocator->GetRenderTextures(device, cmd.uID, cmd.descriptor);
	}
	loadRTCommands.clear();
	for (UINT i = 0; i < unLoadRTCommands.size(); ++i)
	{
		allocator->ReleaseRenderTexutre(unLoadRTCommands[i]);
	}
	unLoadRTCommands.clear();
}