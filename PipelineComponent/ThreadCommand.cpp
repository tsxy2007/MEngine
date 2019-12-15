#include "ThreadCommand.h"
std::mutex ThreadCommand::globalMutex;
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

void ThreadCommand::CollectCommand(std::vector<ID3D12CommandList*>& cmdLists)
{
	ThrowIfFailed(cmdList->Close());
	globalMutex.lock();
	cmdLists.push_back(cmdList.Get());
	globalMutex.unlock();
}

void ThreadCommand::CollectCommand(ID3D12CommandList** cmdListArray, std::atomic_uint* currentLength)
{
	ThrowIfFailed(cmdList->Close());
	cmdListArray[(*currentLength)++] = cmdList.Get();
}

void ThreadCommand::CollectCommand(ID3D12CommandList** cmdListArray, unsigned int* currentLength)
{
	ThrowIfFailed(cmdList->Close());
	cmdListArray[(*currentLength)++] = cmdList.Get();
}


void ThreadCommand::ExecuteCommand(ID3D12CommandQueue* queue)
{
	ThrowIfFailed(cmdList->Close());
	queue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdList.GetAddressOf());
}