#pragma once
#include "../taskflow/taskflow.hpp"
#include <vector>
#include "../RenderComponent/RenderTexture.h"
class ThreadCommand;
class FrameResource;
class PipelineComponent
{
private:
	std::vector<RenderTexture*> allTempRT;
	void AddAllTempRT(RenderTexture** rts, UINT length);
protected:
	ThreadCommand* threadCommand;
	virtual std::vector<RenderTextureDescriptor>& SendRenderTextureRequire() = 0;
	RenderTexture* GetTempRT(UINT index);
	virtual tf::Task RenderPreEvent(tf::Taskflow& taskFlow) = 0;
	virtual tf::Task RenderPostEvent(tf::Taskflow& taskFlow) = 0;
public:
	void InitThreadCommand(ID3D12Device* device);
	void ExecuteThreadCommand(std::vector<ID3D12CommandList*>& executableCommands);
	virtual ~PipelineComponent() {}
};