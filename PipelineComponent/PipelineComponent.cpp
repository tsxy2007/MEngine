#include "PipelineComponent.h"
#include "../Singleton/FrameResource.h"
#include "TempRTAllocator.h"
std::mutex PipelineComponent::mtx;
JobBucket* PipelineComponent::bucket (nullptr);
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

void PipelineComponent::CreateFence(ID3D12Device* device)
{
	if (fence == nullptr && GetCommandListType() != CommandListType_None)
	{
		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_SHARED,
			IID_PPV_ARGS(&fence)));
	}
	dependingComponentCount++;
}

void PipelineComponent::ClearHandles()
{
	jobHandles.clear();
}
void PipelineComponent::MarkHandles()
{
	for (auto ite = cpuDepending.begin(); ite != cpuDepending.end(); ++ite)
	{
		auto& vec = (*ite)->jobHandles;
		for (auto selfIte = jobHandles.begin(); selfIte != jobHandles.end(); ++selfIte)
		{
			for (auto dependIte = vec.begin(); dependIte != vec.end(); ++dependIte)
			{
				dependIte->Precede(*selfIte);
			}
		}
	}
}
PipelineComponent::PipelineComponent()
{
	loadRTCommands.reserve(10);
	requiredRTs.reserve(10);
	unLoadRTCommands.reserve(10);
	jobHandles.reserve(10);
	gpuDepending.reserve(10);
	cpuDepending.reserve(10);
}