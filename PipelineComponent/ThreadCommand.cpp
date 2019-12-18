#include "ThreadCommand.h"
void ThreadCommand::ResetCommand()
{
	ThrowIfFailed(cmdAllocator->Reset());
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));
}
ThreadCommand::ThreadCommand(ID3D12Device* device)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&cmdAllocator)));
	ThrowIfFailed(device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdAllocator.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(&cmdList)));
	cmdList->Close();
}
