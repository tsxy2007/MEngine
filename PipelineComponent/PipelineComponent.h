#pragma once
#include <vector>
#include "../RenderComponent/RenderTexture.h"
#include "../PipelineComponent/IPerCameraResource.h"
#include "../Common/Camera.h"
#include <unordered_map>
#include "../JobSystem/JobSystem.h"
class ThreadCommand;
class FrameResource;
class RenderPipeline;
class TempRTAllocator;
class World;
struct TemporalRTCommand
{
	enum CommandType
	{
		Create, Require
	};
	CommandType type;
	UINT uID;
	RenderTextureDescriptor descriptor;
	bool operator=(const TemporalRTCommand& other) const;
};
class PerCameraRenderingEvent;
class PipelineComponent
{
	friend class RenderPipeline;
	friend class PerCameraRenderingEvent;
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
	struct EventData
	{
		ID3D12Device* device;
		ID3D12Resource* backBuffer;
		Camera* camera;
		FrameResource* resource;
		World* world;
		D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	};
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) = 0;
	virtual void Dispose() = 0;
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt) = 0;
	virtual bool NeedCommandList() const = 0;
	virtual void RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList) = 0;
	~PipelineComponent() {}
};