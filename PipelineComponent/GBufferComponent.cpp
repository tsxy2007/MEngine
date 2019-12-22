#include "GBufferComponent.h"
#include "PrepareComponent.h"
class GBufferRunnable
{
public:
	PipelineComponent::EventData data;
	ThreadCommand* tcmd;

	void operator()()
	{
		tcmd->ResetCommand();
		ID3D12GraphicsCommandList* commandList = tcmd->GetCmdList();
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(data.backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		commandList->ClearRenderTargetView(data.backBufferHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(data.backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		tcmd->CloseCommand();
	}
};
JobHandle GBufferComponent::RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList)
{
	GBufferRunnable runnable
	{
		data,
		commandList
	};
	JobHandle tsk = taskFlow.GetTask<GBufferRunnable>(std::move(runnable));
	return tsk;
}
std::vector<std::string> GBufferComponent::GetDependedEvent()
{
	std::vector<std::string> depending =
	{
		typeid(PrepareComponent).name()
	};
	return depending;
}