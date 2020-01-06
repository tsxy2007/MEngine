#include "PostProcessingComponent.h"
#include "../Singleton/PSOContainer.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/Graphics.h"
#include "../LogicComponent/World.h"
#include "../RenderComponent/Texture.h"
Shader* postShader;
std::unique_ptr<PSOContainer> postContainer;
class PostFrameData : public IPipelineResource
{
public:
	DescriptorHeap postSRVHeap;
	PostFrameData(ID3D12Device* device)
	{
		postSRVHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);
	}
};
ObjectPtr<Texture> tex;
class PostRunnable
{
public:
	RenderTexture* renderTarget;
	ThreadCommand* threadCmd;
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	ID3D12Resource* backBuffer;
	ID3D12Device* device;
	UINT width;
	UINT height;
	PostProcessingComponent* selfPtr;
	FrameResource* resource;
	void operator()()
	{
		threadCmd->ResetCommand();
		PostFrameData* frameRes = (PostFrameData*)selfPtr->resContainer.GetResource(
			&resource->resourceManager,
			selfPtr,
			[=]()->PostFrameData*
		{
			return new PostFrameData(device);
		}).GetResource();
		tex->BindColorBufferToSRVHeap(&frameRes->postSRVHeap, 0, device);
		ID3D12GraphicsCommandList* commandList = threadCmd->GetCmdList();
		Graphics::TransformBackBufferState<BackBufferState_RenderTarget>(commandList, backBuffer);
		postShader->BindRootSignature(commandList);
		postShader->SetResource(commandList, ShaderID::GetMainTex(), &frameRes->postSRVHeap, 0);
		Graphics::Blit(
			commandList,
			device,
			&backBufferHandle,
			1,
			nullptr,
			postContainer.get(),
			width, height,
			postShader,
			0
		);
		Graphics::TransformBackBufferState<BackBufferState_Present>(commandList, backBuffer);
		threadCmd->CloseCommand();
	}
};

void PostProcessingComponent::RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList)
{
	taskFlow.GetTask<PostRunnable>({
		GetTempRT(0),
		commandList,
		data.backBufferHandle,
		data.backBuffer,
		data.device,
		data.world->windowWidth,
		data.world->windowHeight,
		this,
		data.resource
	});
}

void PostProcessingComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	tempRT.resize(1);
	tempRT[0].type = TemporalRTCommand::Require;
	tempRT[0].uID = ShaderID::PropertyToID("_CameraRenderTarget");
	postShader = ShaderCompiler::GetShader("PostProcess");
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	postContainer = std::unique_ptr<PSOContainer>(
		new PSOContainer(DXGI_FORMAT_UNKNOWN, 1, &backBufferFormat)
		);
	tex = new Texture(commandList, device, "woodCrateTex", L"Textures/WoodCrate01.dds");
}

void PostProcessingComponent::Dispose()
{
	postContainer = nullptr;
	tex->Destroy();
}