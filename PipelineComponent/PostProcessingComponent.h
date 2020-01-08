#pragma once
#include "PipelineComponent.h"
class PostRunnable;
class PostProcessingComponent : public PipelineComponent
{
	friend class PostRunnable;
protected:
	std::vector<TemporalRTCommand> tempRT;
	virtual bool NeedCommandList() const { return true; }
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt)
	{
		return tempRT;
	}
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList);
public:
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
};