#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
class UploadBuffer : public MObject
{
public:
	void Create(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, size_t stride);
	UploadBuffer() : MObject() {}
    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	virtual ~UploadBuffer();
	void CopyData(UINT elementIndex, const void* data);
	void CopyDatas(UINT startElementIndex, UINT elementCount, const void* data);
	size_t GetStride() const { return mStride; }
	size_t GetAlignedStride() const { return mElementByteSize; }
	ID3D12Resource* Resource()const
	{
		return mUploadBuffer.Get();
	}
private:
	struct UploadCommand
	{
		UINT startIndex;
		UINT count;
	};
	void* mMappedData;
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	size_t mStride;
	UINT mElementCount;
    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};