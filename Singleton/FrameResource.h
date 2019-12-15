#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/MathHelper.h"
#include "../RenderComponent/UploadBuffer.h"
#include "../RenderComponent/CBufferPool.h"
#include <functional>
#include "../PipelineComponent/ThreadCommand.h"
#include "../Common/Pool.h"
struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexC;
};


struct ObjectConstants
{
	DirectX::XMFLOAT4X4 objectToWorld = MathHelper::Identity4x4();
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 worldSpaceCameraPos;
	float NearZ = 0.0f;
	float FarZ = 0.0f;
};


// Stores the resources needed for the CPU to build the command lists
// for a frame.  
struct FrameResource
{
private:
	static Pool<ThreadCommand> threadCommandMemoryPool;
	std::vector<std::shared_ptr<std::function<void()>>> afterFlushEvents;
public:
	static std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	static FrameResource* mCurrFrameResource;
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();
	void UpdateBeforeFrame(ID3D12Fence* mFence);
	void AddAfterFlushEvent(std::shared_ptr<std::function<void()>>& func);
	void UpdateAfterFrame(UINT64& currentFence, ID3D12CommandQueue* commandQueue, ID3D12Fence* mFence);
	ThreadCommand* GetNewThreadCommand(ID3D12Device* device);
	void ReleaseThreadCommand(ThreadCommand* targetCmd);
    // We cannot reset the allocator until the GPU is done processing the commands.
    // So each frame needs their own allocator.
    //Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;
	std::vector<ThreadCommand*> threadCommands;
    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it.  So each frame needs their own cbuffers.
   // std::unique_ptr<UploadBuffer<FrameConstants>> FrameCB = nullptr;
	std::unordered_map<UINT, ConstBufferElement> cameraCBs;
	std::unordered_map<UINT, ConstBufferElement> objectCBs;
    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
    UINT64 Fence = 0;
};