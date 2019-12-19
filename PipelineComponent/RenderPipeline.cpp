#include "RenderPipeline.h"
#include "../Common/Camera.h"

void RenderPipeline::RenderCamera(ID3D12Device* device, ID3D12CommandQueue* commandQueue, FrameResource* resource, std::vector<Camera*>& allCameras, tf::Executor& executor)
{
	for (UINT camIndex = 0; camIndex < allCameras.size(); ++camIndex)
	{
		Camera* cam = allCameras[camIndex];
		std::vector<PipelineComponent*> waitingComponents = renderPathComponents[(UINT)cam->GetRenderingPath()];
		allPipelineTasks.clear();
		for (UINT i = 0; i < waitingComponents.size(); ++i)
		{ 
			PipelineComponent* component = waitingComponents[i];
			component->InitThreadCommand(device, cam, resource);
			std::vector<RenderTextureDescriptor>& descriptors = component->SendRenderTextureRequire();
			//Allocate Temporal Render Texture
			component->allTempRT.resize(descriptors.size());
			tempRTAllocator.GetRenderTextures(device, descriptors.data(), component->allTempRT.data(), component->allTempRT.size());
			allPipelineTasks[component] = component->RenderEvent(taskFlow);
			component->ExecuteThreadCommand(commandLists, cam);//Add Command Lists to waiting vector
		}
		//TODO
		//Build events dependency
	}
	//Thread Sync, CPU Busy, Main Thread sleep
	executor.run(taskFlow).wait();
	//Final Execute
	commandQueue->ExecuteCommandLists(commandLists.size(), commandLists.data());
	//Finalize Frame
	commandLists.clear();
	taskFlow.clear();
	tempRTAllocator.CumulateReleaseAfterFrame();
}