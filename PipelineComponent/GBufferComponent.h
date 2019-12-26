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
	virtual void RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList);
public:
	virtual void Initialize();
	virtual void Dispose() {};
};

