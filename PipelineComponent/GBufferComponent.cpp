#include "GBufferComponent.h"
#include "PrepareComponent.h"
#include "../Common/d3dUtil.h"
#include "../Singleton/Graphics.h"
#include "../LogicComponent/World.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../Singleton/PSOContainer.h"
PSOContainer* gbufferContainer(nullptr);
#define ALBEDO_RT(component) (component->GetTempRT(0))
#define SPECULAR_RT(component) (component->GetTempRT(1))
#define NORMAL_RT (component) (component->GetTempRT(2))
#define EMISSION_RT (component) (component->GetTempRT(3))
#define MOTION_VECTOR_RT (component) (component->GetTempRT(4))
class GBufferRunnable
{
public:
	ID3D12Resource* backBuffer;
	ID3D12Device* device;
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	ThreadCommand* tcmd;
	Camera* cam;
	FrameResource* resource;
	GBufferComponent* component;
	void operator()()
	{
		tcmd->ResetCommand();
		ID3D12GraphicsCommandList* commandList = tcmd->GetCmdList();
		ID3D12Device* thsDevice = device;
	//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		tcmd->CloseCommand();
	}
};

std::vector<TemporalRTCommand>& GBufferComponent::SendRenderTextureRequire(EventData& evt) {
	for (int i = 0; i < tempRTRequire.size(); ++i)
	{
		tempRTRequire[i].descriptor.width = evt.world->windowWidth;
		tempRTRequire[i].descriptor.height = evt.world->windowHeight;
	}
	return tempRTRequire;
}
void GBufferComponent::RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList)
{
	GBufferRunnable runnable
	{
		data.backBuffer,
		data.device,
		data.backBufferHandle,
		commandList,
		data.camera,
		data.resource,
		this
	};
	taskFlow.GetTask(runnable);
}

void GBufferComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	tempRTRequire.resize(5);
	TemporalRTCommand& albedoBuffer = tempRTRequire[0];
	albedoBuffer.type = TemporalRTCommand::Create;
	albedoBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture0");
	albedoBuffer.descriptor.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	albedoBuffer.descriptor.depthType = RenderTextureDescriptor::None;
	albedoBuffer.descriptor.depthSlice = 1;
	albedoBuffer.descriptor.type = RenderTextureType::Tex2D;

	TemporalRTCommand& specularBuffer = tempRTRequire[1];
	specularBuffer.type = TemporalRTCommand::Create;
	specularBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture1");
	specularBuffer.descriptor.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	specularBuffer.descriptor.depthType = RenderTextureDescriptor::None;
	specularBuffer.descriptor.depthSlice = 1;
	specularBuffer.descriptor.type = RenderTextureType::Tex2D;

	TemporalRTCommand& normalBuffer = tempRTRequire[2];
	normalBuffer.type = TemporalRTCommand::Create;
	normalBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture2");
	normalBuffer.descriptor.colorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	normalBuffer.descriptor.depthType = RenderTextureDescriptor::None;
	normalBuffer.descriptor.depthSlice = 1;
	normalBuffer.descriptor.type = RenderTextureType::Tex2D;

	TemporalRTCommand& emissionBuffer = tempRTRequire[3];
	emissionBuffer.type = TemporalRTCommand::Create;
	emissionBuffer.uID = ShaderID::PropertyToID("_CameraRenderTarget");
	emissionBuffer.descriptor.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	emissionBuffer.descriptor.depthType = RenderTextureDescriptor::DepthStencil;
	emissionBuffer.descriptor.depthSlice = 1;
	emissionBuffer.descriptor.type = RenderTextureType::Tex2D;

	TemporalRTCommand& motionVectorBuffer = tempRTRequire[4];
	motionVectorBuffer.type = TemporalRTCommand::Create;
	motionVectorBuffer.uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	motionVectorBuffer.descriptor.colorFormat = DXGI_FORMAT_R16G16_SNORM;
	motionVectorBuffer.descriptor.depthType = RenderTextureDescriptor::None;
	motionVectorBuffer.descriptor.depthSlice = 1;
	motionVectorBuffer.descriptor.type = RenderTextureType::Tex2D;
	
	std::vector<DXGI_FORMAT> colorFormats(tempRTRequire.size());
	for (int i = 0; i < tempRTRequire.size(); ++i)
	{
		colorFormats[i] = tempRTRequire[i].descriptor.colorFormat;
	}
	gbufferContainer = new PSOContainer(DXGI_FORMAT_D24_UNORM_S8_UINT, colorFormats.size(), colorFormats.data());
}

void GBufferComponent::Dispose()
{
	delete gbufferContainer;
}