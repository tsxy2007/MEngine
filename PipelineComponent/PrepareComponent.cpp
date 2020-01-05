#include "PrepareComponent.h"
#include "../Common/Camera.h"
#include "../RenderComponent/UploadBuffer.h"
struct PrepareRunnable
{
	PrepareComponent* ths;
	ThreadCommand* threadCommand;
	FrameResource* resource;
	Camera* camera;
	void operator()()
	{
		camera->UploadCameraBuffer(ths->passConstants);
	}
};

void PrepareComponent::RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList)
{
	PrepareRunnable runnable =
	{
		this,
		commandList,
		data.resource,
		data.camera
	};
	taskHandle = taskFlow.GetTask(runnable);
	FrameResource* res = data.resource;
	Camera* cam = data.camera;
	PassConstants* psConstPtr = &passConstants;
	taskHandle.Precede(std::forward<JobHandle>(
		taskFlow.GetTask([=]()->void {
		ConstBufferElement ele = res->cameraCBs[cam->GetInstanceID()];
		ele.buffer->CopyData(ele.element, psConstPtr);
	}
	)));
}