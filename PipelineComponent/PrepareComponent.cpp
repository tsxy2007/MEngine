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
		ConstBufferElement ele = resource->cameraCBs[camera->GetInstanceID()];
		ele.buffer->CopyData(ele.element,&ths->passConstants);
	}
};

void PrepareComponent::RenderEvent(EventData& data,  ThreadCommand* commandList)
{
	PrepareRunnable runnable =
	{
		this,
		commandList,
		data.resource,
		data.camera
	};
	ScheduleJob(runnable);
}