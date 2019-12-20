#include "PipelineComponent.h"
#include "../Singleton/FrameResource.h"
std::mutex PipelineComponent::mtx;
RenderTexture* PipelineComponent::GetTempRT(UINT index)
{
	return allTempRT[index];
}