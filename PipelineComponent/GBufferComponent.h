#pragma once
#include "PipelineComponent.h"
class GBufferRunnable;
class PSOContainer;
class StructuredBuffer;
class PrepareComponent;
class GBufferComponent : public PipelineComponent
{
	friend class GBufferRunnable;
protected:
	std::vector<TemporalRTCommand> tempRTRequire;
	virtual bool NeedCommandList() const { return true; }
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt);
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList);
public:
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();

};

