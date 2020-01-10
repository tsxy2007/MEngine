#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>

class CommandBuffer
{
private:
	ID3D12CommandQueue* graphicsCommandQueue;
	ID3D12CommandQueue* computeCommandQueue;
	ID3D12CommandQueue* asyncQueue;
	std::vector<ID3D12GraphicsCommandList*> graphicsCmdLists;
	std::vector<ID3D12GraphicsCommandList*> asyncCmdLists;
	struct Fence
	{
		ID3D12Fence* fence;
		UINT64 frameIndex;
	};
	struct ExecuteCommand
	{
		UINT start;
		UINT count;
	};
	struct InnerCommand
	{
		enum CommandType
		{
			CommandType_ExecuteGraphics,
			CommandType_ExecuteCompute,
			CommandType_WaitCompute,
			CommandType_SignalCompute,
			CommandType_WaitGraphics,
			CommandType_SignalGraphics
		};
		CommandType type;
		union
		{
			UINT executeCount;
			Fence waitFence;
			Fence signalFence;
		};
	};
	std::vector<InnerCommand> executeCommands;
	enum ExecuteType
	{
		NOT_EXECUTING,
		EXECUTING_GRAPHICS,
		EXECUTING_COMPUTE
	};
	ExecuteType executeChoose = NOT_EXECUTING;
	UINT executeCount = 0;
	void ChangeExecuteState(ExecuteType state);
public:
	ID3D12CommandQueue* GetGraphicsQueue() const {
		return graphicsCommandQueue;
	}
	ID3D12CommandQueue* GetComputeQueue() const {
		return computeCommandQueue;
	}
	ID3D12CommandQueue* GetAsyncQueue() const {
		return asyncQueue;
	}
	void WaitForCompute(ID3D12Fence* computeFence, UINT currentFrame);
	void WaitForGraphics(ID3D12Fence* graphicsFence, UINT currentFrame);
	void SignalToCompute(ID3D12Fence* computeFence, UINT currentFrame);
	void SignalToGraphics(ID3D12Fence* graphicsFence, UINT currentFrame);
	void ExecuteGraphicsCommandList(ID3D12GraphicsCommandList* commandList);
	void ExecuteComputeCommandList(ID3D12GraphicsCommandList* commandList);
	void ExecuteAsyncComputeCommandList(ID3D12GraphicsCommandList* commandList);
	void Submit();
	void Clear();
	CommandBuffer(
		ID3D12CommandQueue* graphicsCommandQueue,
		ID3D12CommandQueue* computeCommandQueue,
		ID3D12CommandQueue* asyncQueue
	);
};