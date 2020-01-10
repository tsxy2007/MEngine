#include "CommandBuffer.h"

void CommandBuffer::ChangeExecuteState(ExecuteType state)
{
	if (state == executeChoose) return;
	InnerCommand cmd;
	switch (state)
	{
	case NOT_EXECUTING:
		if (executeChoose == EXECUTING_GRAPHICS)
		{
			cmd.type = InnerCommand::CommandType_ExecuteGraphics;
		}
		else//Executing compute
		{
			cmd.type = InnerCommand::CommandType_ExecuteCompute;
		}
		cmd.executeCount = executeCount;
		executeCount = 0;
		executeCommands.push_back(cmd);

		break;
	case EXECUTING_GRAPHICS:
		if (executeChoose == EXECUTING_COMPUTE)
		{
			cmd.type = InnerCommand::CommandType_ExecuteCompute;
			cmd.executeCount = executeCount;
			executeCount = 0;
			executeCommands.push_back(cmd);
		}
		break;
	case EXECUTING_COMPUTE:
		if (executeChoose == EXECUTING_GRAPHICS)
		{
			cmd.type = InnerCommand::CommandType_ExecuteGraphics;
			cmd.executeCount = executeCount;
			executeCount = 0;
			executeCommands.push_back(cmd);
		}
		break;
	}
	executeChoose = state;
}
void CommandBuffer::ExecuteAsyncComputeCommandList(ID3D12GraphicsCommandList* commandList)
{
	asyncCmdLists.push_back(commandList);
}
void CommandBuffer::Clear()
{
	executeCommands.clear();
	graphicsCmdLists.clear();
	asyncCmdLists.clear();
	executeCount = 0;
	executeChoose = NOT_EXECUTING;
}
void CommandBuffer::Submit()
{
	ChangeExecuteState(NOT_EXECUTING);
	UINT graphicsIndex = 0;
	if (!asyncCmdLists.empty())
	{
		asyncQueue->ExecuteCommandLists(asyncCmdLists.size(), (ID3D12CommandList**)asyncCmdLists.data());
	}
	for (auto ite = executeCommands.begin(); ite != executeCommands.end(); ++ite)
	{
		switch (ite->type)
		{
		case InnerCommand::CommandType_ExecuteGraphics:
			graphicsCommandQueue->ExecuteCommandLists(ite->executeCount, (ID3D12CommandList**)(graphicsCmdLists.data() + graphicsIndex));
			graphicsIndex += ite->executeCount;
			break;
		case InnerCommand::CommandType_ExecuteCompute:
			computeCommandQueue->ExecuteCommandLists(ite->executeCount, (ID3D12CommandList**)(graphicsCmdLists.data() + graphicsIndex));
			graphicsIndex += ite->executeCount;
			break;
		case InnerCommand::CommandType_WaitGraphics:
			graphicsCommandQueue->Wait(ite->waitFence.fence, ite->waitFence.frameIndex);
			break;
		case InnerCommand::CommandType_SignalCompute:
			computeCommandQueue->Signal(ite->signalFence.fence, ite->signalFence.frameIndex);
			break;
		case InnerCommand::CommandType_WaitCompute:
			computeCommandQueue->Wait(ite->waitFence.fence, ite->waitFence.frameIndex);
			break;
		case InnerCommand::CommandType_SignalGraphics:
			graphicsCommandQueue->Signal(ite->signalFence.fence, ite->signalFence.frameIndex);
			break;
		}
	}
}

void CommandBuffer::WaitForCompute(ID3D12Fence* computeFence, UINT currentFrame)
{
	ChangeExecuteState(NOT_EXECUTING);
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_WaitCompute;
	cmd.waitFence.fence = computeFence;
	cmd.waitFence.frameIndex = currentFrame;
	executeCommands.push_back(cmd);
}
void CommandBuffer::WaitForGraphics(ID3D12Fence* computeFence, UINT currentFrame)
{
	ChangeExecuteState(NOT_EXECUTING);
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_WaitGraphics;
	cmd.waitFence.fence = computeFence;
	cmd.waitFence.frameIndex = currentFrame;
	executeCommands.push_back(cmd);
}
void CommandBuffer::SignalToCompute(ID3D12Fence* computeFence, UINT currentFrame)
{
	ChangeExecuteState(NOT_EXECUTING);
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_SignalCompute;
	cmd.signalFence.fence = computeFence;
	cmd.signalFence.frameIndex = currentFrame;
	executeCommands.push_back(cmd);
}
void CommandBuffer::SignalToGraphics(ID3D12Fence* computeFence, UINT currentFrame)
{
	ChangeExecuteState(NOT_EXECUTING);
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_SignalGraphics;
	cmd.signalFence.fence = computeFence;
	cmd.signalFence.frameIndex = currentFrame;
	executeCommands.push_back(cmd);
}
void CommandBuffer::ExecuteGraphicsCommandList(ID3D12GraphicsCommandList* commandList)
{
	ChangeExecuteState(EXECUTING_GRAPHICS);
	graphicsCmdLists.push_back(commandList);
	executeCount++;
}
void CommandBuffer::ExecuteComputeCommandList(ID3D12GraphicsCommandList* commandList)
{
	ChangeExecuteState(EXECUTING_COMPUTE);
	graphicsCmdLists.push_back(commandList);
	executeCount++;
}

CommandBuffer::CommandBuffer(
	ID3D12CommandQueue* graphicsCommandQueue,
	ID3D12CommandQueue* computeCommandQueue,
	ID3D12CommandQueue* asyncQueue
) : graphicsCommandQueue(graphicsCommandQueue),
computeCommandQueue(computeCommandQueue),
asyncQueue(asyncQueue)
{
	asyncCmdLists.reserve(20);
	graphicsCmdLists.reserve(20);
}