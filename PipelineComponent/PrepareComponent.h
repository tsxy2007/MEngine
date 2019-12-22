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
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire() { return useless; }
	virtual JobHandle RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList);
	virtual std::vector<std::string> GetDependedEvent()
	{
		std::vector<std::string> useless;
		return useless;
	}
};
