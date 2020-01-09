#pragma once
#include "PipelineComponent.h"
#include "../Singleton/FrameResource.h"
class Camera;
class PrepareRunnable;
class MeshRenderer;
class PrepareComponent : public PipelineComponent
{
	friend class PrepareRunnable;
public:
	PassConstants passConstants;
	DirectX::XMFLOAT4 frustumPlanes[6];
protected:
	std::vector<TemporalRTCommand> useless;
	virtual CommandListType GetCommandListType() {
		return CommandListType_None;
	}
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt) { return useless; }
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList);
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {

	};
	virtual void Dispose() {};
};
