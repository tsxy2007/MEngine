#pragma once
#include "PipelineComponent.h"
#include "../RenderComponent/MeshRenderer.h"
class GBufferRunnable;
class GBufferComponent : public PipelineComponent
{
	friend class GBufferRunnable;
protected:

	std::vector<TemporalRTCommand> tempRTRequire;
	virtual bool NeedCommandList() const { return true; }
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt);
	virtual JobHandle RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList);
	virtual std::vector<std::string> GetDependedEvent();
public:
	GBufferComponent();
};

