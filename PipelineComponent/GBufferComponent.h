#pragma once
#include "PipelineComponent.h"
class GBufferRunnable;
class GBufferComponent : public PipelineComponent
{
	friend class GBufferRunnable;
protected:
	std::vector<TemporalRTCommand> useless;
	virtual bool NeedCommandList() const { return true; }
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire() { return useless; }
	virtual tf::Task RenderEvent(EventData& data, tf::Taskflow& taskFlow, ThreadCommand* tCmd);
	virtual std::vector<std::string> GetDependedEvent();
};

