#pragma once
#include <vector>
#include "../RenderComponent/RenderTexture.h"
#include "../PipelineComponent/IPerCameraResource.h"
#include "../Common/Camera.h"
#include <unordered_map>
#include "../JobSystem/JobSystem.h"
#include "RenderPipeline.h"
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
class CommandBuffer;
class PipelineComponent
{
	friend class RenderPipeline;
	friend class PerCameraRenderingEvent;
private:
	static std::mutex mtx;
	static JobBucket* bucket;
	ThreadCommand* threadCommand;//thread command cache
	std::vector<RenderTexture*> allTempRT;
	struct LoadTempRTCommand
	{
		UINT uID;
		UINT index;
		RenderTextureDescriptor descriptor;
		LoadTempRTCommand(UINT uID, UINT index, RenderTextureDescriptor& descriptor) :
			uID(uID), index(index), descriptor(descriptor) {}
	};
	std::vector<JobHandle> jobHandles;
	std::vector<LoadTempRTCommand> loadRTCommands;
	std::vector<UINT> unLoadRTCommands;
	std::vector<PipelineComponent*> dependedComponents;
	std::vector<std::pair<UINT, UINT>> requiredRTs;
	void ExecuteTempRTCommand(ID3D12Device* device, TempRTAllocator* allocator);
	template <typename ... T>
	class Depending;

	template <>
	class Depending<>
	{
	public:
		Depending(std::vector<PipelineComponent*>&) {}
	};

	template <typename T, typename ... Args>
	class Depending<T, Args...>
	{
	public:
		Depending(std::vector<PipelineComponent*>& vec)
		{
			vec.push_back(RenderPipeline::GetComponent<T>());
			Depending<Args...> d(vec);
		}
	};
protected:
	RenderTexture* GetTempRT(UINT index);
	template <typename... Args>
	void SetDepending()
	{
		Depending<Args...> d(dependedComponents);
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
		CommandBuffer* commandBuffer;
		UINT64 frameNum;
		UINT width, height;
		bool isBackBufferForPresent;
	};
	template <typename Func>
	JobHandle ScheduleJob(Func&& func)
	{
		JobHandle handle = bucket->GetTask(std::move(func));
		jobHandles.push_back(handle);
		return handle;
	}

	template <typename Func>
	JobHandle ScheduleJob(Func& func)
	{
		return ScheduleJob(std::move(func));
	}
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) = 0;
	virtual void Dispose() = 0;
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt) = 0;
	virtual bool NeedCommandList() const = 0;
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList) = 0;
	void ClearHandles();
	void MarkHandles();
	~PipelineComponent() {}
	PipelineComponent();
};