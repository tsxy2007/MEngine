#pragma once
#include "../Common/Camera.h"
#include "../JobSystem/JobInclude.h"
#include "RenderPipeline.h"
class ThreadCommand;
class FrameResource;
class TempRTAllocator;
class RenderTexture;
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
enum CommandListType
{
	CommandListType_None,
	CommandListType_Graphics,
	CommandListType_Compute
};
class PipelineComponent
{
	friend class RenderPipeline;
	friend class PerCameraRenderingEvent;
private:

	static std::mutex mtx;
	static JobBucket* bucket;
	ThreadCommand* threadCommand;//thread command cache
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
	std::vector<PipelineComponent*> cpuDepending;
	std::vector<PipelineComponent*> gpuDepending;
	std::vector<std::pair<UINT, UINT>> requiredRTs;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	UINT dependingComponentCount = 0;
	void CreateFence(ID3D12Device* device);
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
	std::vector<RenderTexture*> allTempRT;
	template <typename... Args>
	void SetCPUDepending()
	{
		Depending<Args...> d(cpuDepending);
	}
	template <typename... Args>
	void SetGPUDepending()
	{
		Depending<Args...> d(gpuDepending);
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
	virtual CommandListType GetCommandListType() = 0;
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) = 0;
	virtual void Dispose() = 0;
	virtual std::vector<TemporalRTCommand>& SendRenderTextureRequire(EventData& evt) = 0;
	virtual void RenderEvent(EventData& data, ThreadCommand* commandList) = 0;
	void ClearHandles();
	void MarkHandles();
	~PipelineComponent() {}
	PipelineComponent();
};