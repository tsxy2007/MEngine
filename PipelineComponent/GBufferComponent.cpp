#include "GBufferComponent.h"
#include "PrepareComponent.h"
#include "../Common/d3dUtil.h"
#include "../Singleton/Graphics.h"
#include "../LogicComponent/World.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Common/DescriptorHeap.h"
#include "../RenderComponent/Skybox.h"
Shader* postProcessingShader(nullptr);
Skybox* sky;
PSOContainer* gbufferContainer(nullptr);
PSOContainer* postProcessContainer(nullptr);
UINT _MainTex;
class GBufferFrameResource : public IPerCameraResource
{
public:
	ObjectPtr<DescriptorHeap> rtHeap;
	GBufferFrameResource(ID3D12Device* device)
	{
		rtHeap = new DescriptorHeap();
		rtHeap->Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);
	}
	virtual ~GBufferFrameResource()
	{
		if (rtHeap != nullptr)
			rtHeap->Destroy();
	}

};
class GBufferRunnable
{
public:
	ID3D12Resource* backBuffer;
	ID3D12Device* device;
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	ThreadCommand* tcmd;
	RenderTexture* rt;
	Camera* cam;
	FrameResource* resource;
	GBufferComponent* component;
	void operator()()
	{
		tcmd->ResetCommand();
		ID3D12GraphicsCommandList* commandList = tcmd->GetCmdList();
		rt->ClearRenderTarget(commandList, 0, true, true);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		/*	UINT64 n64RequiredSize = 0u;
		UINT   nNumSubresources = 1u;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT stTxtLayouts = {};
		UINT64 n64TextureRowSizes = 0u;
		UINT   nTextureRowNum = 0u;
		D3D12_RESOURCE_DESC destDesc = backBuffer->GetDesc();
		device->GetCopyableFootprints(&destDesc
			, 0
			, nNumSubresources
			, 0
			, &stTxtLayouts
			, &nTextureRowNum
			, &n64TextureRowSizes
			, &n64RequiredSize);
		CD3DX12_TEXTURE_COPY_LOCATION Dst(backBuffer, 0);
		CD3DX12_TEXTURE_COPY_LOCATION Src(rt->GetColorResource(), stTxtLayouts);
		commandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);*/
		//commandList->ClearRenderTargetView(backBufferHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);

		//sky->Draw()
		//postProcessingShader->BindRootSignature(commandList);
		//Graphics::Blit(commandList, device, &backBufferHandle, 1, nullptr, gbufferContainer, rt->GetWidth(), rt->GetHeight(), postProcessingShader, 0);
		ID3D12Device* thsDevice = device;
		auto getPerFrameResource = [=]()->IPerCameraResource*
		{
			return new GBufferFrameResource(thsDevice);
		};
		GBufferFrameResource* frameResource = (GBufferFrameResource*)resource->GetResource(component, getPerFrameResource);
		rt->BindColorBufferToHeap(frameResource->rtHeap.operator->(), 0, device);
		//Draw Skybox
		rt->SetViewport(commandList);
		rt->ClearRenderTarget(commandList, 0, true, true);
		commandList->OMSetRenderTargets(1, &rt->GetColorDescriptor(0), true, &rt->GetDepthDescriptor(0));
		sky->Draw(
			0,
			commandList,
			device,
			&resource->cameraCBs[cam->GetInstanceID()],
			resource,
			gbufferContainer
		);
		//Post Processing
		postProcessingShader->BindRootSignature(commandList);
		DescriptorHeap* heap = frameResource->rtHeap.operator->();
		ID3D12DescriptorHeap* heapPtr = heap->Get().Get();
		commandList->SetDescriptorHeaps(1, &heapPtr);
		postProcessingShader->SetResource(commandList, _MainTex, heap, 0);
		Graphics::Blit(
			commandList,
			device,
			&backBufferHandle,
			1,
			nullptr,
			postProcessContainer,
			rt->GetWidth(),
			rt->GetHeight(),
			postProcessingShader,
			0
		);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
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
		data.device,
		data.backBufferHandle,
		commandList,
		GetTempRT(0),
		data.camera,
		data.resource,
		this
	};
	taskFlow.GetTask(runnable);
	//	return tsk;
}

void GBufferComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
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
	gbufferContainer = new PSOContainer(DXGI_FORMAT_D24_UNORM_S8_UINT, 1, &cmd.descriptor.colorFormat);
	postProcessContainer = new PSOContainer(DXGI_FORMAT_UNKNOWN, 1, &cmd.descriptor.colorFormat);
	postProcessingShader = ShaderCompiler::GetShader("PostProcess");
	ObjectPtr<Texture> tex = new Texture(commandList, device, "grasscube1024", L"Textures/grasscube1024.dds", Texture::Cubemap);;
	sky = new Skybox(tex, device, commandList);
	_MainTex = ShaderID::PropertyToID("_MainTex");
}

void GBufferComponent::Dispose()
{
	delete gbufferContainer;
	delete postProcessContainer;
}