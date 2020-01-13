#include "PostProcessingComponent.h"
#include "../Singleton/PSOContainer.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/Graphics.h"
#include "../LogicComponent/World.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "TemporalAA.h"
Shader* postShader;
std::unique_ptr<PSOContainer> backBufferContainer;
std::unique_ptr<PSOContainer> renderTextureContainer;
std::unique_ptr<TemporalAA> taaComponent;
//PrepareComponent* prepareComp = nullptr;
class PostFrameData : public IPipelineResource
{
public:
	DescriptorHeap postSRVHeap;
	PostFrameData(ID3D12Device* device)
	{
		postSRVHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);
	}
};
class PostRunnable
{
public:
	RenderTexture* renderTarget;
	RenderTexture* motionVector;
	RenderTexture* destMap;
	ThreadCommand* threadCmd;
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	ID3D12Resource* backBuffer;
	ID3D12Device* device;
	UINT width;
	UINT height;
	void* selfPtr;
	FrameResource* resource;
	bool isForPresent;
	Camera* cam;
	void operator()()
	{
		threadCmd->ResetCommand();
		ID3D12GraphicsCommandList* commandList = threadCmd->GetCmdList();
		PostFrameData* frameRes = (PostFrameData*)resource->GetPerCameraResource(selfPtr, cam,
			[=]()->PostFrameData*
		{
			return new PostFrameData(device);
		});
		taaComponent->Run(
			renderTarget,
			renderTarget,
			motionVector,
			destMap,
			commandList,
			resource,
			cam,
			width, height
		);
		destMap->BindColorBufferToSRVHeap(&frameRes->postSRVHeap, 0, device);

		if (isForPresent)
		{
			Graphics::ResourceStateTransform(
				commandList, 
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET, 
				backBuffer);
		}
		postShader->BindRootSignature(commandList, &frameRes->postSRVHeap);
		postShader->SetResource(commandList, ShaderID::GetMainTex(), &frameRes->postSRVHeap, 0);
		Graphics::Blit(
			commandList,
			device,
			&backBufferHandle,
			1,
			nullptr,
			backBufferContainer.get(),
			width, height,
			postShader,
			0
		);
		if (isForPresent) {
			Graphics::ResourceStateTransform(
				commandList,
				D3D12_RESOURCE_STATE_RENDER_TARGET, 
				D3D12_RESOURCE_STATE_PRESENT,
				backBuffer);
		}
		threadCmd->CloseCommand();
	}
};

void PostProcessingComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	JobHandle handle = ScheduleJob<PostRunnable>({
		(RenderTexture*)allTempResource[0],
		(RenderTexture*)allTempResource[1],
		(RenderTexture*)allTempResource[2],
		commandList,
		data.backBufferHandle,
		data.backBuffer,
		data.device,
		data.width,
		data.height,
		this,
		data.resource,
		data.isBackBufferForPresent,
		data.camera
		});
}
void PostProcessingComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<PrepareComponent>();

	tempRT.resize(3);
	tempRT[0].type = TemporalResourceCommand::Require;
	tempRT[0].uID = ShaderID::PropertyToID("_CameraRenderTarget");
	tempRT[1].type = TemporalResourceCommand::Require;
	tempRT[1].uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	tempRT[2].type = TemporalResourceCommand::Create;
	tempRT[2].uID = ShaderID::PropertyToID("_PostProcessBlitTarget");
	auto& desc = tempRT[2].descriptor;
	desc.rtDesc.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.rtDesc.depthSlice = 1;
	desc.rtDesc.depthType = RenderTextureDepthSettings_None;
	desc.rtDesc.width = 0;
	desc.rtDesc.height = 0;
	desc.rtDesc.type = RenderTextureType_Tex2D;
	postShader = ShaderCompiler::GetShader("PostProcess");
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	backBufferContainer = std::unique_ptr<PSOContainer>(
		new PSOContainer(DXGI_FORMAT_UNKNOWN, 1, &backBufferFormat)
		);
	DXGI_FORMAT rtFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	renderTextureContainer = std::unique_ptr<PSOContainer>(
		new PSOContainer(DXGI_FORMAT_UNKNOWN, 1, &rtFormat)
		);
	taaComponent = std::unique_ptr<TemporalAA>(new TemporalAA());
	prepareComp = RenderPipeline::GetComponent<PrepareComponent>();
	taaComponent->prePareComp = prepareComp;
	taaComponent->device = device;
	taaComponent->toRTContainer = renderTextureContainer.get();
}

 std::vector<TemporalResourceCommand>& PostProcessingComponent::SendRenderTextureRequire(EventData& evt)
{
	auto& desc = tempRT[2].descriptor;
	desc.rtDesc.width = evt.width;
	desc.rtDesc.height = evt.height;
	return tempRT;
}

void PostProcessingComponent::Dispose()
{
	backBufferContainer = nullptr;
	taaComponent = nullptr;
}