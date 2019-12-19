#pragma once
#include "../taskflow/taskflow.hpp"
#include <vector>
#include "../RenderComponent/RenderTexture.h"
class ThreadCommand;
class FrameResource;
class RenderPipeline;
class Camera;
class PipelineComponent
{
	friend class RenderPipeline;
private:
	std::vector<RenderTexture*> allTempRT;
protected:
	ThreadCommand* threadCommand = nullptr;
	virtual std::vector<RenderTextureDescriptor>& SendRenderTextureRequire() = 0;
	RenderTexture* GetTempRT(UINT index);
	virtual tf::Task RenderEvent(tf::Taskflow& taskFlow) = 0;
public:
	void InitThreadCommand(ID3D12Device* device, Camera* cam, FrameResource* resource);
	void ExecuteThreadCommand(std::vector<ID3D12CommandList*>& executableCommands, Camera* cam);
	virtual ~PipelineComponent() {}
};