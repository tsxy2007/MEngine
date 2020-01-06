#include "SkyboxComponent.h"
#include "../Singleton/ShaderID.h"
#include "../RenderComponent/Skybox.h"
#include "../Singleton/PSOContainer.h"
#include "RenderPipeline.h"
#include "PrepareComponent.h"
Skybox* defaultSkybox;
std::unique_ptr<PSOContainer> psoContainer;
class SkyboxPerFrameData : public IPipelineResource
{
public:
	DescriptorHeap heap;
	SkyboxPerFrameData(ID3D12Device* device)
	{
		heap.Create(device,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	}
};
PrepareComponent* prepareComp;
class SkyboxRunnable
{
public:
	RenderTexture* gbufferTex;
	RenderTexture* mvTex;
	SkyboxComponent* selfPtr;
	ThreadCommand* commandList;
	FrameResource* resource;
	ID3D12Device* device;
	Camera* cam;
	void operator()()
	{
		commandList->ResetCommand();
		if (psoContainer == nullptr ||
			psoContainer->GetColorFormats()[0] != gbufferTex->GetColorFormat() ||
			psoContainer->GetColorFormats()[1] != mvTex->GetColorFormat() ||
			psoContainer->GetDepthFormat() != gbufferTex->GetDepthFormat())
		{
			DXGI_FORMAT rtFormats[2];
			rtFormats[0] = gbufferTex->GetColorFormat();
			rtFormats[1] = mvTex->GetColorFormat();
			psoContainer = std::unique_ptr<PSOContainer>(
				new PSOContainer(gbufferTex->GetDepthFormat(), 2, rtFormats)
				);
		}
		SkyboxPerFrameData* frameData = (SkyboxPerFrameData*)selfPtr->container.GetResource(&resource->resourceManager, selfPtr, [&]()->SkyboxPerFrameData*
		{
			return new SkyboxPerFrameData(device);
		}).GetResource();
		gbufferTex->BindRTVToHeap(device, &frameData->heap, 0, 0);
		mvTex->BindRTVToHeap(device, &frameData->heap, 1, 0);
		ID3D12GraphicsCommandList* cmdList = commandList->GetCmdList();
		gbufferTex->SetViewport(cmdList);
		D3D12_CPU_DESCRIPTOR_HANDLE rtHandles[2];
		rtHandles[0] = frameData->heap.hCPU(0);
		rtHandles[1] = frameData->heap.hCPU(1);
		D3D12_CPU_DESCRIPTOR_HANDLE depthHandle = gbufferTex->GetDepthDescriptor(0);
		cmdList->OMSetRenderTargets(
			2,
			rtHandles,
			true,
			&depthHandle
		);
		defaultSkybox->Draw(
			0,
			commandList->GetCmdList(),
			device,
			&resource->cameraCBs[cam->GetInstanceID()],
			resource,
			psoContainer.get()
		);
		commandList->CloseCommand();
	}
};
void SkyboxComponent::RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList)
{
	JobHandle hand = taskFlow.GetTask<SkyboxRunnable>(
		{
			 GetTempRT(0),
			 GetTempRT(1),
			 this,
			 commandList,
			 data.resource,
			 data.device,
			 data.camera
		});
	//prepareComp->taskHandle.Precede(hand);
}


void SkyboxComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	prepareComp = (PrepareComponent*)RenderPipeline::GetComponent(typeid(PrepareComponent).name());
	tempRT.resize(2);
	tempRT[0].type = TemporalRTCommand::Require;
	tempRT[0].uID = ShaderID::PropertyToID("_CameraRenderTarget");
	tempRT[1].type = TemporalRTCommand::Require;
	tempRT[1].uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	ObjectPtr<Texture> skyboxTexture = new Texture(
		commandList,
		device,
		"grasscube1024",
		L"Textures/grasscube1024.dds", 
		Texture::Cubemap
	);
	defaultSkybox = new Skybox(
		skyboxTexture,
		device,
		commandList
	);
}
void SkyboxComponent::Dispose()
{
	psoContainer = nullptr;
	delete defaultSkybox;
}