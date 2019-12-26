#include "GBufferComponent.h"
#include "PrepareComponent.h"
#include "../Common/d3dUtil.h"
#include "../LogicComponent/World.h"
class GBufferRunnable
{
public:
	ID3D12Resource* backBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	ThreadCommand* tcmd;
	RenderTexture* rt;
	void operator()()
	{
		tcmd->ResetCommand();
		ID3D12GraphicsCommandList* commandList = tcmd->GetCmdList();
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		commandList->ClearRenderTargetView(backBufferHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		int width = rt->GetWidth();
		int height = rt->GetHeight();
		tcmd->CloseCommand();
	}
};

std::vector<TemporalRTCommand>& GBufferComponent::SendRenderTextureRequire(EventData& evt) { 
	tempRTRequire[0].descriptor.width = evt.world->windowWidth;
	tempRTRequire[0].descriptor.height = evt.world->windowHeight;
	return tempRTRequire;
}
void GBufferComponent::RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList)
{
	GBufferRunnable runnable
	{
		data.backBuffer,
		data.backBufferHandle,
		commandList,
		GetTempRT(0)
	};
	taskFlow.GetTask(runnable);
//	return tsk;
}

void GBufferComponent::Initialize()
{
	tempRTRequire.resize(1);
	TemporalRTCommand& cmd = tempRTRequire[0];
	cmd.type = TemporalRTCommand::Create;
	cmd.uID = ShaderID::PropertyToID("_CameraRT");
	cmd.descriptor.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	cmd.descriptor.depthFormat = DXGI_FORMAT_R24G8_TYPELESS;
	cmd.descriptor.depthSlice = 1;
	cmd.descriptor.height = 800;
	cmd.descriptor.width = 600;
	cmd.descriptor.type = RenderTextureType::Tex2D;
}