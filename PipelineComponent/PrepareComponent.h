#pragma once
#include "PipelineComponent.h"
#include "../Singleton/FrameResource.h"
class Camera;
class PrepareRunnable;
class MeshRenderer;
class CameraData : public IPipelineResource
{
public:
	DirectX::XMMATRIX vpMatrix;
	DirectX::XMMATRIX inverseVpMatrix;
	PassConstants passConstants;
};
class PrepareComponent : public PipelineComponent
{
	friend class PrepareRunnable;
public:
	JobHandle taskHandle;
protected:
	std::vector<TemporalRTCommand> useless;
	virtual bool NeedCommandList() const { return false; }
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt) { return useless; }
	virtual void RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList);
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {};
	virtual void Dispose() {};
};
