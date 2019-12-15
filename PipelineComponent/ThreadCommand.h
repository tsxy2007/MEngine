#pragma once
#include "../Common/d3dUtil.h"
#include <mutex>
#include <atomic>
#include <vector>
#include "../taskflow/taskflow.hpp"
class ThreadCommand final
{
private:
	static std::mutex globalMutex;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
public:
	inline ID3D12CommandAllocator* GetAllocator() const { return cmdAllocator.Get(); }
	inline ID3D12GraphicsCommandList* GetCmdList() const { return cmdList.Get(); }
	ThreadCommand(ID3D12Device* device);
	void ResetCommand();
	void CollectCommand(std::vector<ID3D12CommandList*>& cmdLists);
	void CollectCommand(ID3D12CommandList** cmdListArray, std::atomic_uint* currentLength);
	void CollectCommand(ID3D12CommandList** cmdListArray, unsigned int* currentLength);
	void ExecuteCommand(ID3D12CommandQueue* queue);
};