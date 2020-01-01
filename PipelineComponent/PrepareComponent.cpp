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
		CameraData* camData = (CameraData*)camera->GetResource(ths, []()->CameraData*
		{
			return new CameraData;
		});
		camera->UploadCameraBuffer(resource, camData->passConstants);
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
}