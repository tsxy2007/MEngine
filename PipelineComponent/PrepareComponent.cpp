#include "PrepareComponent.h"
#include "../Common/Camera.h"
#include "../RenderComponent/UploadBuffer.h"
struct PrepareRunnable
{
	PrepareComponent* ths;
	ThreadCommand* threadCommand;
	PipelineComponent::EventData data;
	void operator()()
	{
		data.camera->UploadCameraBuffer(data.resource, ths->currentCameraData);
	}
};

void PrepareComponent::RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList)
{
	PrepareRunnable runnable =
	{
		this,
		commandList,
		data
	};
	JobHandle tsk = taskFlow.GetTask(runnable);
}

PassConstants const* PrepareComponent::GetCameraData()
{
	return &currentCameraData;
}