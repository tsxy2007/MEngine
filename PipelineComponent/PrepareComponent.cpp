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
		threadCommand->ResetCommand();
		auto cmdList = threadCommand->GetCmdList();
		data.camera->UploadCameraBuffer(data.resource, ths->currentCameraData);
		UploadBuffer::UploadData(cmdList);
		threadCommand->CloseCommand();
	}
};

JobHandle PrepareComponent::RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList)
{
	PrepareRunnable runnable =
	{
		this,
		commandList,
		data
	};
	JobHandle tsk = taskFlow.GetTask(runnable);
	return tsk;
}

PassConstants const* PrepareComponent::GetCameraData()
{
	return &currentCameraData;
}