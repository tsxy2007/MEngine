#include "LightingComponent.h"
#include "PrepareComponent.h"
#include "../RenderComponent/Light.h"
#include "CameraData/CameraTransformData.h"
#include "RenderPipeline.h"
#include "../Singleton/MathLib.h"
using namespace DirectX;
std::vector<LightCommand> lights;
struct LightCullCBuffer
{
	XMFLOAT4X4 _InvVP;
	XMFLOAT4 _CameraNearPos;
	XMFLOAT4 _CameraFarPos;
	XMFLOAT4 _ZBufferParams;
	XMFLOAT3 _CameraForward;
	UINT _LightCount;
};
void LightingComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<PrepareComponent>();
	lights.reserve(50);
	prepareComp = RenderPipeline::GetComponent<PrepareComponent>();
	
}
void LightingComponent::Dispose()
{
}
std::vector<TemporalResourceCommand>& LightingComponent::SendRenderTextureRequire(EventData& evt)
{
	return tempResources;
}

class LightFrameData : public IPipelineResource
{
public:
	UploadBuffer lightsInFrustum;
	LightFrameData(ID3D12Device* device)
	{
		lightsInFrustum.Create(device, 50, false, sizeof(LightCommand));
	}
};

struct LightingRunnable
{
	ThreadCommand* tcmd;
	ID3D12Device* device;
	Camera* cam;
	FrameResource* res;
	void* selfPtr;
	PrepareComponent* prepareComp;
	float clusterLightFarPlane;
	void operator()()
	{
		tcmd->ResetCommand();
		auto commandList = tcmd->GetCmdList();
		XMVECTOR vec[6];
		memcpy(vec, prepareComp->frustumPlanes, sizeof(XMVECTOR) * 6);
		XMVECTOR camForward = cam->GetLook();
		vec[0] = MathLib::GetPlane(std::move(camForward), std::move(cam->GetPosition() + min(cam->GetFarZ(), clusterLightFarPlane) * camForward));
		Light::GetLightingList(lights, 
			vec,
			std::move(prepareComp->frustumMinPos),
			std::move(prepareComp->frustumMaxPos));
		LightFrameData* lightData = (LightFrameData*)res->GetPerCameraResource(selfPtr, cam, [&]()->LightFrameData*
		{
			return new LightFrameData(device);
		});
		if (lightData->lightsInFrustum.GetElementCount() < lights.size())
		{
			UINT maxSize = max(lights.size(), (UINT)(lightData->lightsInFrustum.GetElementCount() * 1.5));
			lightData->lightsInFrustum.Create(device, maxSize, false, sizeof(LightCommand));
		}
		lightData->lightsInFrustum.CopyDatas(0, lights.size(), lights.data());
		tcmd->CloseCommand();
	}
};
void LightingComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<LightingRunnable>({
		commandList,
		data.device,
		data.camera,
		data.resource,
		this,
		prepareComp,
		128
	});
}