#pragma once
#include "PipelineComponent.h"
#include "../RenderComponent/MeshRenderer.h"
class GBufferRunnable;
class GBufferComponent : public PipelineComponent
{
	friend class GBufferRunnable;
protected:
	ObjectPtr<MeshRenderer> testBox;
	ObjectPtr<Mesh> boxMesh;
	ObjectPtr<Material> testMat;
	ObjectPtr<DescriptorHeap> heap;
	ObjectPtr<Texture> mainTexture;
	std::vector<TemporalRTCommand> useless;
	virtual bool NeedCommandList() const { return true; }
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire() { return useless; }
	virtual JobHandle RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList);
	virtual std::vector<std::string> GetDependedEvent();
public:
	//GBufferComponent();
};

