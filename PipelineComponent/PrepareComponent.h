#pragma once
#include "PipelineComponent.h"
#include "../Singleton/FrameResource.h"
class Camera;
class PrepareRunnable;
class PrepareComponent : public PipelineComponent
{
	friend class PrepareRunnable;
public:
	PassConstants const* GetCameraData();
protected:
	PassConstants currentCameraData;
	std::vector<TemporalRTCommand> useless;
	virtual bool NeedCommandList() const { return true; }
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt) { return useless; }
	virtual void RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList);
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {};
	virtual void Dispose() {};
};
