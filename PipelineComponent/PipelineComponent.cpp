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
	for (auto ite = loadRTCommands.begin(); ite != loadRTCommands.end(); ++ite)
	{
		LoadTempRTCommand& cmd = *ite;
		allTempRT[cmd.index] = allocator->GetRenderTextures(device, cmd.uID, cmd.descriptor);
	}
	loadRTCommands.clear();
	for (auto ite = requiredRTs.begin(); ite != requiredRTs.end(); ++ite)
	{
		allTempRT[ite->first] = allocator->GetUsingRenderTexture(ite->second);
	}
	requiredRTs.clear();
	for (auto ite = unLoadRTCommands.begin(); ite!= unLoadRTCommands.end(); ++ite)
	{
		allocator->ReleaseRenderTexutre(*ite);
	}
	unLoadRTCommands.clear();
}

bool TemporalRTCommand::operator=(const TemporalRTCommand& other) const
{
	bool eq = type == other.type && uID == other.uID;
	if (type == Create)
	{
		return eq && descriptor == other.descriptor;
	}
	else
	{
		return eq;
	}
}