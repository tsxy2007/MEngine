#include "UploadBuffer.h"
#include "../Singleton/FrameResource.h"
std::vector<UploadBuffer*> UploadBuffer::needUpdateLists;
void UploadBuffer::Create(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, size_t stride, bool isUAV)
{
	mIsWritable = true;
	needUpdateElements.reserve(min(10, elementCount));
	mIsConstantBuffer = isConstantBuffer;
	// Constant buffer elements need to be multiples of 256 bytes.
	// This is because the hardware can only view constant data 
	// at m*256 byte offsets and of n*256 byte lengths. 
	// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
	// UINT64 OffsetInBytes; // multiple of 256
	// UINT   SizeInBytes;   // multiple of 256
	// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
	if (isConstantBuffer)
		mElementByteSize = d3dUtil::CalcConstantBufferByteSize(stride);
	else mElementByteSize = stride;
	mStride = stride;
	mIsUAV = isUAV;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mUploadBuffer)));
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		isUAV ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mDefaultBuffer)));
	ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));


	// We do not need to unmap until we are done with the resource.  However, we must not write to
	// the resource while it is in use by the GPU (so we must use synchronization techniques).
}

UploadBuffer::~UploadBuffer()
{
	if (mUploadBuffer != nullptr)
		mUploadBuffer->Unmap(0, nullptr);
}

void UploadBuffer::MakeNoLongerWritable()
{
	if (!mIsWritable) return;
	mIsWritable = false;
	if (mUploadBuffer.Get() == nullptr) return;
	FrameResource::ReleaseResourceAfterFlush(mUploadBuffer);
	if (mUploadBuffer != nullptr)
		mUploadBuffer->Unmap(0, nullptr);
	if (!isDirty)
		mUploadBuffer = nullptr;
}

void UploadBuffer::CopyData(UINT elementIndex, const void* data)
{
	if (!mIsWritable) return;
	char* dataPos = (char*)mMappedData;
	size_t offset = elementIndex * mElementByteSize;
	dataPos += offset;
	memcpy(dataPos, data, mStride);
	localMtx.lock();
	if (!isDirty)
	{
		isDirty = true;
		needUpdateLists.push_back(this);
	}
	needUpdateElements.push_back({ (UINT)elementIndex, 1 });
	localMtx.unlock();
}

void UploadBuffer::CopyDatas(UINT startElementIndex, UINT elementCount, const void* data)
{
	if (!mIsWritable) return;
	char* dataPos = (char*)mMappedData;
	size_t offset = startElementIndex * mElementByteSize;
	dataPos += offset;
	memcpy(dataPos, data, (elementCount - 1) * mElementByteSize + mStride);
	localMtx.lock();
	if (!isDirty)
	{
		isDirty = true;
		needUpdateLists.push_back(this);
	}
	needUpdateElements.push_back({ startElementIndex, elementCount });
	localMtx.unlock();
}

void UploadBuffer::UploadData(ID3D12GraphicsCommandList* commandList)
{
	for (int i = 0; i < needUpdateLists.size(); ++i)
	{
		needUpdateLists[i]->UploadDataToDefaultBuffer(commandList);
	}
	needUpdateLists.clear();
}

void UploadBuffer::SetUAV(bool isUAV, ID3D12GraphicsCommandList* cmdList)
{
	localMtx.lock();
	if (mIsUAV == isUAV)
	{
		localMtx.unlock();
		return;
	}
	mIsUAV = isUAV;
	localMtx.unlock();
	if (isUAV)
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDefaultBuffer.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}
	else
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDefaultBuffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_GENERIC_READ));
	}
}

void UploadBuffer::UploadDataToDefaultBuffer(ID3D12GraphicsCommandList* commandList)
{
	isDirty = false;
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDefaultBuffer.Get(),
		mIsUAV ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_COPY_DEST));
	for (int i = 0; i < needUpdateElements.size(); ++i)
	{
		UploadCommand& command = needUpdateElements[i];
		size_t offset = command.startIndex * mElementByteSize;
		commandList->CopyBufferRegion(mDefaultBuffer.Get(), offset, mUploadBuffer.Get(), offset, (command.count - 1) * mElementByteSize + mStride);
	}
	if (!mIsWritable)
		mUploadBuffer = nullptr;
	needUpdateElements.clear();
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDefaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		mIsUAV ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_GENERIC_READ));

}