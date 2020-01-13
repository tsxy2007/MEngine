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
	XMFLOAT4 _ScreenParams;		//x is the width of the camera¡¯s target texture in pixels, y is the height of the camera¡¯s target texture in pixels, z is 1.0 + 1.0 / width and w is 1.0 + 1.0 / height.
	XMFLOAT4 _ZBufferParams;		// x is (1-far/near), y is (far/near), z is (x/far) and w is (y/far).
	//Align
	XMFLOAT3 _TemporalClipBounding;
	float _Sharpness;
	//Align
	XMFLOAT2 _Jitter;
	XMFLOAT2 _LastJitter;
	//Align
	UINT _MainTexIndex;
	UINT _LastRtTexIndex;
	UINT _LastDepthIndex;
	UINT _LastMotionIndex;
	//Align
	UINT _MotionVectorIndex;
	UINT _DepthTexIndex;

};

struct TAAFrameData : public IPipelineResource
{
	UploadBuffer taaBuffer;
	DescriptorHeap srvHeap;
	TAAFrameData(ID3D12Device* device)
	{
		taaBuffer.Create(device, 1, true, sizeof(TAAConstBuffer));
		srvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 6, true);
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
public:
	PrepareComponent* prePareComp;
	ID3D12Device* device;
	PSOContainer* toRTContainer;
	UINT TAAConstBuffer_Index;
	Shader* taaShader;
	TemporalAA()
	{
		TAAConstBuffer_Index = ShaderID::PropertyToID("TAAConstBuffer");
		taaShader = ShaderCompiler::GetShader("TemporalAA");
	}
	void Run(
		RenderTexture* inputColorBuffer,
		RenderTexture* inputDepthBuffer,
		RenderTexture* motionVector,
		RenderTexture* renderTargetTex,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* res,
		Camera* cam,
		UINT width, UINT height)
	{
		CameraTransformData* camTransData = (CameraTransformData*)cam->GetResource(prePareComp, [&]()->CameraTransformData*
		{
			return new CameraTransformData;
		});
		TAACameraData* tempCamData = (TAACameraData*)cam->GetResource(this, [&]()->TAACameraData*
		{
			return new TAACameraData(width, height, device);
		});
		TAAFrameData* tempFrameData = (TAAFrameData*)res->GetPerCameraResource(this, cam, [&]()->TAAFrameData*
		{
			return new TAAFrameData(device);
		});
		if (tempCamData->UpdateFrame(width, height, device))
		{
			//Refresh & skip
			Graphics::CopyTexture(commandList, inputColorBuffer, CopyTarget_ColorBuffer, renderTargetTex, CopyTarget_ColorBuffer);
		}
		else
		{
			TAAConstBuffer constBufferData;
			inputColorBuffer->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 0, device);
			constBufferData._MainTexIndex = 0;
			inputDepthBuffer->BindDepthBufferToSRVHeap(&tempFrameData->srvHeap, 1, device);
			constBufferData._DepthTexIndex = 1;
			motionVector->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 2, device);
			constBufferData._MotionVectorIndex = 2;
			tempCamData->lastDepthTexture->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 3, device);
			constBufferData._LastDepthIndex = 3;
			tempCamData->lastMotionVectorTexture->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 4, device);
			constBufferData._LastMotionIndex = 4;
			tempCamData->lastRenderTarget->BindColorBufferToSRVHeap(&tempFrameData->srvHeap, 5, device);
			constBufferData._LastRtTexIndex = 5;
			constBufferData._InvNonJitterVP = *(XMFLOAT4X4*)&XMMatrixInverse(&XMMatrixDeterminant(camTransData->nonJitteredVPMatrix), camTransData->nonJitteredVPMatrix);
			constBufferData._InvLastVp = *(XMFLOAT4X4*)&XMMatrixInverse(&XMMatrixDeterminant(camTransData->lastVP), camTransData->lastVP);
			constBufferData._FinalBlendParameters = { 0.95f,0.85f, 6000.0f, 0 };		//Stationary Move, Const, 0
			constBufferData._CameraDepthTexture_TexelSize = { 1.0f / width, 1.0f / height, (float)width, (float) height };
			constBufferData._ScreenParams = { (float)width, (float)height, 1.0f / width + 1, 1.0f / height + 1 };
			constBufferData._ZBufferParams.x = (float)(1.0 - (cam->GetFarZ() / cam->GetNearZ()));
			constBufferData._ZBufferParams.y = (float)(cam->GetFarZ() / cam->GetNearZ());
			constBufferData._ZBufferParams.z = constBufferData._ZBufferParams.x / cam->GetFarZ();
			constBufferData._ZBufferParams.w = constBufferData._ZBufferParams.y / cam->GetFarZ();
			constBufferData._TemporalClipBounding = { 3.0f, 1.25f, 3000.0f };
			constBufferData._Sharpness = 0.1f;
			constBufferData._Jitter = camTransData->jitter;
			constBufferData._LastJitter = camTransData->lastFrameJitter;
			tempFrameData->taaBuffer.CopyData(0, &constBufferData);
			taaShader->BindRootSignature(commandList, &tempFrameData->srvHeap);
			taaShader->SetResource(commandList, TAAConstBuffer_Index, &tempFrameData->taaBuffer, 0);
			taaShader->SetResource(commandList, ShaderID::GetMainTex(), &tempFrameData->srvHeap, 0);
			Graphics::Blit(
				commandList, 
				device,
				&renderTargetTex->GetColorDescriptor(0),1,
				nullptr, 
				toRTContainer,
				width, height,
				taaShader, 0
			);
		}
		Graphics::CopyTexture(commandList, renderTargetTex, CopyTarget_ColorBuffer, tempCamData->lastRenderTarget.get(), CopyTarget_ColorBuffer);
		Graphics::CopyTexture(commandList, motionVector, CopyTarget_ColorBuffer, tempCamData->lastMotionVectorTexture.get(), CopyTarget_ColorBuffer);
		Graphics::CopyTexture(commandList, inputDepthBuffer, CopyTarget_DepthBuffer, tempCamData->lastDepthTexture.get(), CopyTarget_ColorBuffer);
	}
};