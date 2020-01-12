#pragma once
#include "../Singleton/PSOContainer.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/Graphics.h"
#include "../LogicComponent/World.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "CameraData/CameraTransformData.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/RenderTexture.h"
#include "PrepareComponent.h"
using namespace DirectX;		//Only for single .cpp, namespace allowed
struct TAAConstBuffer
{
	XMFLOAT4X4 _InvNonJitterVP;
	XMFLOAT4X4 _InvLastVp;
	XMFLOAT4 _FinalBlendParameters;
	XMFLOAT4 _CameraDepthTexture_TexelSize;
	//Align
	XMFLOAT3 _TemporalClipBounding;
	float _Sharpness;
	//Align
	XMFLOAT2 _Jitter;
	XMFLOAT2 _LastJitter;
	//Align
	UINT mainTexIndex;
	UINT lastRtTexIndex;
	UINT lastDepthIndex;
	UINT lastMotionIndex;
	//Align
	UINT motionVectorIndex;
	UINT depthTexIndex;
};

struct TAAFrameData : public IPipelineResource
{
	UploadBuffer taaBuffer;
	DescriptorHeap srvHeap;
	TAAFrameData(ID3D12Device* device)
	{
		taaBuffer.Create(device, 1, true, sizeof(TAAConstBuffer));
		srvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10, true);
	}
};

struct TAACameraData : public IPipelineResource
{
	std::unique_ptr<RenderTexture> lastRenderTarget;
	std::unique_ptr<RenderTexture> lastDepthTexture;
	std::unique_ptr<RenderTexture> lastMotionVectorTexture;
	UINT width, height;
private:
	void Update(UINT width, UINT height, ID3D12Device* device)
	{
		this->width = width;
		this->height = height;
		lastRenderTarget = std::unique_ptr<RenderTexture>(
			new RenderTexture(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, RenderTextureDepthSettings_None, RenderTextureType_Tex2D, 0, 0)
			);
		lastDepthTexture = std::unique_ptr<RenderTexture>(
			new RenderTexture(device, width, height, DXGI_FORMAT_R32_FLOAT, RenderTextureDepthSettings_None, RenderTextureType_Tex2D, 0, 0)
			);
		lastMotionVectorTexture = std::unique_ptr<RenderTexture>(
			new RenderTexture(device, width, height, DXGI_FORMAT_R16G16_SNORM, RenderTextureDepthSettings_None, RenderTextureType_Tex2D, 0, 0)
			);
	}
public:
	TAACameraData(UINT width, UINT height, ID3D12Device* device)
	{
		Update(width, height, device);
	}

	bool UpdateFrame(UINT width, UINT height, ID3D12Device* device)
	{
		if (width != this->width || height != this->height)
		{
			Update(width, height, device);
			return true;
		}
		return false;
	}
};

class TemporalAA
{
private:

public:
	void Run(
		RenderTexture* renderTarget,
		RenderTexture* motionVector,
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		FrameResource* res,
		PrepareComponent* prePareComp,
		Camera* cam,
		PipelineComponent* currentComp, UINT width, UINT height)
	{
		CameraTransformData* camTransData = (CameraTransformData*)cam->GetResource(prePareComp, [&]()->CameraTransformData*
		{
			return new CameraTransformData;
		});
		TAACameraData* tempCamData = (TAACameraData*)cam->GetResource(currentComp, [&]()->TAACameraData*
		{
			return new TAACameraData(width, height, device);
		});
		TAAFrameData* tempFrameData = (TAAFrameData*)res->GetPerCameraResource(currentComp, cam, [&]()->TAAFrameData*
		{
			return new TAAFrameData(device);
		});
		if (tempCamData->UpdateFrame(width, height, device))
		{
			
		}
		else
		{

		}
		Graphics::CopyTexture(commandList, renderTarget, CopyTarget_ColorBuffer, tempCamData->lastRenderTarget.get(), CopyTarget_ColorBuffer);
		Graphics::CopyTexture(commandList, motionVector, CopyTarget_ColorBuffer, tempCamData->lastMotionVectorTexture.get(), CopyTarget_ColorBuffer);
		Graphics::CopyTexture(commandList, renderTarget, CopyTarget_DepthBuffer, tempCamData->lastDepthTexture.get(), CopyTarget_ColorBuffer);
	}
};