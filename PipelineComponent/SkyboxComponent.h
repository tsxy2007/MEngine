#pragma once
#include "PipelineComponent.h"
class SkyboxComponent : public PipelineComponent
{
protected:
	std::vector<TemporalRTCommand> tempRT;
	virtual bool NeedCommandList() const { return true; }
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt)
	{
		return tempRT;
	}
	virtual void RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList);
public:
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
};

