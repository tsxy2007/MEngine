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
	virtual bool NeedCommandList() const = 0;
	virtual tf::Task RenderEvent(ID3D12Device* device, tf::Taskflow& taskFlow) = 0;
	virtual std::vector<std::string> GetDependedEvent() = 0;
public:
	void InitThreadCommand(ID3D12Device* device, Camera* cam, FrameResource* resource);
	void ExecuteThreadCommand(std::vector<ID3D12CommandList*>& executableCommands, Camera* cam);
	virtual ~PipelineComponent() {}
};

class TestComponent : public PipelineComponent
{
protected:
	std::vector<RenderTextureDescriptor> sb;
	virtual bool NeedCommandList() const { return false; }
	virtual std::vector<RenderTextureDescriptor>& SendRenderTextureRequire() { return sb; }
	virtual tf::Task RenderEvent(ID3D12Device* device, tf::Taskflow& taskFlow) { tf::Task t; return t; }
	virtual std::vector<std::string> GetDependedEvent() { std::vector<std::string>  t; return t; }
};