#include "RenderCommand.h"
std::mutex RenderCommand::mtx;
RingQueue<std::shared_ptr<RenderCommand>> RenderCommand::queue(100);

void RenderCommand::AddCommand(RenderCommand* command)
{
	std::lock_guard<std::mutex> lck(mtx);
	queue.EmplacePush(command);
}

void RenderCommand::ExecuteCommand(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	FrameResource* resource)
{
	std::shared_ptr<RenderCommand> cmd = nullptr;
	{
		std::lock_guard<std::mutex> lck(mtx);
		queue.TryPop(&cmd);
	}
	(*cmd)(device, commandList, resource);
}