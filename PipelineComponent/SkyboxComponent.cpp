#include "SkyboxComponent.h"
#include "../Singleton/ShaderID.h"
#include "../RenderComponent/Skybox.h"
#include "../Singleton/PSOContainer.h"
#include "RenderPipeline.h"
Skybox* defaultSkybox;
std::unique_ptr<PSOContainer> psoContainer;
using namespace DirectX;
class SkyboxPerFrameData : public IPipelineResource
{
public:
	UploadBuffer posBuffer;
	SkyboxPerFrameData(ID3D12Device* device)
	{
		posBuffer.Create(
			device,
			1, true,
			sizeof(XMFLOAT4X4)
		);
	}
};

XMMATRIX CalculateViewMatrix(Camera* cam)
{
	XMVECTOR R = cam->GetRight();
	XMVECTOR U = cam->GetUp();
	XMVECTOR L = cam->GetLook();
	// Keep camera's axes orthogonal to each other and of unit length.
	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L, R));

	// U, L already ortho-normal, so no need to normalize cross product.
	R = XMVector3Cross(U, L);

	XMFLOAT3* mRight = (XMFLOAT3*)&R;
	XMFLOAT3* mUp = (XMFLOAT3*)&U;
	XMFLOAT3* mLook = (XMFLOAT3*)&L;
	XMFLOAT4X4 mView;
	mView(0, 0) = mRight->x;
	mView(1, 0) = mRight->y;
	mView(2, 0) = mRight->z;
	mView(3, 0) = 0;

	mView(0, 1) = mUp->x;
	mView(1, 1) = mUp->y;
	mView(2, 1) = mUp->z;
	mView(3, 1) = 0;

	mView(0, 2) = mLook->x;
	mView(1, 2) = mLook->y;
	mView(2, 2) = mLook->z;
	mView(3, 2) = 0;

	mView(0, 3) = 0.0f;
	mView(1, 3) = 0.0f;
	mView(2, 3) = 0.0f;
	mView(3, 3) = 1.0f;
	return *(XMMATRIX*)&mView;
}

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
		SkyboxPerFrameData* frameData = (SkyboxPerFrameData*)resource->GetPerCameraResource(selfPtr, cam, [&]()->SkyboxPerFrameData*
		{
			return new SkyboxPerFrameData(device);
		});
		XMMATRIX view = CalculateViewMatrix(cam);
		XMMATRIX viewProj = XMMatrixMultiply(view, cam->GetProj());
		XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
		frameData->posBuffer.CopyData(0, &invViewProj);
		ID3D12GraphicsCommandList* cmdList = commandList->GetCmdList();
		gbufferTex->SetViewport(cmdList);
		D3D12_CPU_DESCRIPTOR_HANDLE rtHandles[2];
		rtHandles[0] = gbufferTex->GetColorDescriptor(0);
		rtHandles[1] = mvTex->GetColorDescriptor(0);
		D3D12_CPU_DESCRIPTOR_HANDLE depthHandle = gbufferTex->GetDepthDescriptor(0);
		RenderTexture* gbRT = gbufferTex;
		cmdList->OMSetRenderTargets(
			2,
			rtHandles,
			false,
			&depthHandle
		);
		ConstBufferElement skyboxData;
		skyboxData.buffer = &frameData->posBuffer;
		skyboxData.element = 0;
		defaultSkybox->Draw(
			0,
			commandList->GetCmdList(),
			device,
			&skyboxData,
			resource,
			psoContainer.get()
		);
		commandList->CloseCommand();
	}
};
void SkyboxComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<SkyboxRunnable>(
		{
			 (RenderTexture*)allTempResource[0],
			  (RenderTexture*)allTempResource[1],
			 this,
			 commandList,
			 data.resource,
			 data.device,
			 data.camera
		});
}


void SkyboxComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	tempRT.resize(2);
	tempRT[0].type = TemporalResourceCommand::Require;
	tempRT[0].uID = ShaderID::PropertyToID("_CameraRenderTarget");
	tempRT[1].type = TemporalResourceCommand::Require;
	tempRT[1].uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	ObjectPtr<Texture> skyboxTexture = new Texture(
		commandList,
		device,
		nullptr,
		"grasscube1024",
		L"Textures/grasscube1024.dds",
		true,
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