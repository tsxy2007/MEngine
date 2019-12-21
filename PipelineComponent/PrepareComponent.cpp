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

tf::Task PrepareComponent::RenderEvent(EventData& data, tf::Taskflow& taskFlow, ThreadCommand* tCmd)
{
	PrepareRunnable runnable =
	{
		this,
		tCmd,
		data
	};
	tf::Task tsk = taskFlow.emplace(runnable);
	return tsk;
}

PassConstants const* PrepareComponent::GetCameraData()
{
	return &currentCameraData;
}