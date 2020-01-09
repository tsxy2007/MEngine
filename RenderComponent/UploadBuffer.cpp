#include "UploadBuffer.h"
#include "../Singleton/FrameResource.h"
void UploadBuffer::Create(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, size_t stride)
{
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
	if (mUploadBuffer.Get() != nullptr)
	{
		mUploadBuffer->Unmap(0, nullptr);
		mUploadBuffer = nullptr;
	}
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mUploadBuffer)));
	ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

	// We do not need to unmap until we are done with the resource.  However, we must not write to
	// the resource while it is in use by the GPU (so we must use synchronization techniques).
}

UploadBuffer::~UploadBuffer()
{
	if (mUploadBuffer != nullptr)
		mUploadBuffer->Unmap(0, nullptr);
}

void UploadBuffer::CopyData(UINT elementIndex, const void* data)
{
	char* dataPos = (char*)mMappedData;
	size_t offset = elementIndex * mElementByteSize;
	dataPos += offset;
	memcpy(dataPos, data, mStride);
}

void UploadBuffer::CopyDataInside(UINT from, UINT to)
{
	char* dataPos = (char*)mMappedData;
	size_t fromOffset = from * mElementByteSize;
	size_t toOffset = to * mElementByteSize;
	memcpy(dataPos + toOffset, dataPos + fromOffset, mStride);
}

void* UploadBuffer::GetMappedDataPtr(UINT element)
{
	char* dataPos = (char*)mMappedData;
	size_t offset = element * mElementByteSize;
	dataPos += offset;
	return dataPos;
}

void UploadBuffer::CopyDatas(UINT startElementIndex, UINT elementCount, const void* data)
{
	char* dataPos = (char*)mMappedData;
	size_t offset = startElementIndex * mElementByteSize;
	dataPos += offset;
	memcpy(dataPos, data, (elementCount - 1) * mElementByteSize + mStride);
}


void UploadBuffer::CopyFrom(UploadBuffer* otherBuffer, UINT selfStartIndex, UINT otherBufferStartIndex, UINT elementCount)
{
	char* otherPtr = (char*)otherBuffer->mMappedData;
	char* currPtr = (char*)mMappedData;
	otherPtr += otherBuffer->GetAlignedStride() * otherBufferStartIndex;
	currPtr += GetAlignedStride() * selfStartIndex;
	memcpy(currPtr, otherPtr, elementCount * GetAlignedStride());
}

void  UploadBuffer::ReleaseAfterFlush(FrameResource* res)
{
	FrameResource::ReleaseResourceAfterFlush(mUploadBuffer, res);
}