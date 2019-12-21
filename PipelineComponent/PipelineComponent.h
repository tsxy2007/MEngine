#pragma once
#include "../taskflow/taskflow.hpp"
#include <vector>
#include "../RenderComponent/RenderTexture.h"
#include "../PipelineComponent/IPerCameraResource.h"
#include "../Common/Camera.h"
#include <unordered_map>
class ThreadCommand;
class FrameResource;
class RenderPipeline;
class TempRTAllocator;
struct TemporalRTCommand
{
	enum CommandType
	{
		Create, Require
	};
	CommandType type;
	UINT uID;
	RenderTextureDescriptor descriptor;
	bool operator=(const TemporalRTCommand& other) const
	{
		bool eq = type == other.type && uID == other.uID;
		if (type == Create)
		{
			return eq && descriptor == other.descriptor;
		}
		else
		{
			return eq;
		}
	}
};
class PipelineComponent
{
	friend class RenderPipeline;
private:
	static std::mutex mtx;
	ThreadCommand* threadCommand;//thread command cache
	std::vector<RenderTexture*> allTempRT;
	struct LoadTempRTCommand
	{
		UINT uID;
		UINT index;
		RenderTextureDescriptor descriptor;
	};
	std::vector<LoadTempRTCommand> loadRTCommands;
	std::vector<UINT> unLoadRTCommands;
	void ExecuteTempRTCommand(ID3D12Device* device, TempRTAllocator* allocator);
	template <typename Func>
	static IPerCameraResource* GetResource(Func&& func, PerCameraData& data, void* thisClassPtr)
	{
		std::lock_guard<std::mutex> lck(mtx);
		auto&& ite = data.resources.find((size_t)thisClassPtr);
		if (ite == data.resources.end())
		{
			auto&& ite1 = data.resources.find((size_t)thisClassPtr);
			IPerCameraResource* comp;
			if (ite1 == data.resources.end())
			{
				comp = func();
				data.resources.insert_or_assign((size_t)thisClassPtr, comp);
			}
			else
			{
				comp = ite1->second;
			}
			
			return comp;
		}
		else
		{
			return ite1->second;
		}
	}
protected:
	
	RenderTexture* GetTempRT(UINT index);

	template <typename Func>
	static IPerCameraResource* GetPerCameraResource(Func&& func, Camera* cam, void* thisClassPtr)
	{
		return GetResource<Func>(std::move(func), cam->cameraRenderData, thisClassPtr);
	}
	template <typename Func>
	static IPerCameraResource* GetPerFrameResource(Func&& func, Camera* cam, FrameResource* res, void* thisClassPtr)
	{
		PerCameraData& data = res->perCameraDatas[cam]->camData;
		return GetResource<Func>(std::move(func), data, thisClassPtr);
	}
public:
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire() = 0;
	virtual bool NeedCommandList() const = 0;
	virtual tf::Task RenderEvent(ID3D12Device* device, tf::Taskflow& taskFlow, ThreadCommand* commandList) = 0;
	virtual std::vector<std::string> GetDependedEvent() = 0;
	virtual ~PipelineComponent() {}
};

class TestComponent : public PipelineComponent
{
public:
	std::vector<TemporalRTCommand> sb;
	virtual bool NeedCommandList() const { return false; }
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire() { return sb; }
	virtual tf::Task RenderEvent(ID3D12Device* device, tf::Taskflow& taskFlow, ThreadCommand* commandList) { tf::Task t; return t; }
	virtual std::vector<std::string> GetDependedEvent() { std::vector<std::string>  t; return t; }
};