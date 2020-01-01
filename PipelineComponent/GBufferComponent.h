#pragma once
#include "PipelineComponent.h"
#include "../RenderComponent/MeshRenderer.h"
class GBufferRunnable;
class PSOContainer;
class StructuredBuffer;
class GBufferPerFrameResource : public IPipelineResource
{
public:
	std::unique_ptr<StructuredBuffer> objectBuffer;
	UINT objectIndex;
	GBufferPerFrameResource(ID3D12Device* device, UINT initIndex);

	void Resize(UINT targetSize, ID3D12Device* device);
};
class GBufferComponent : public PipelineComponent
{
	friend class GBufferRunnable;
protected:
	std::vector<TemporalRTCommand> tempRTRequire;
	virtual bool NeedCommandList() const { return true; }
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt);
	virtual void RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList);
public:
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();

};

