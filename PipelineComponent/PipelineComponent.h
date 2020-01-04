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
		LoadTempRTCommand(UINT uID, UINT index, RenderTextureDescriptor& descriptor) :
			uID(uID), index(index), descriptor(descriptor) {}
	};
	std::vector<LoadTempRTCommand> loadRTCommands;
	std::vector<UINT> unLoadRTCommands;
	std::vector<std::pair<UINT, UINT>> requiredRTs;
	void ExecuteTempRTCommand(ID3D12Device* device, TempRTAllocator* allocator);

protected:
	RenderTexture* GetTempRT(UINT index);
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