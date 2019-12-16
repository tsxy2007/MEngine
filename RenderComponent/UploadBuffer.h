#pragma once

#include "../Common/d3dUtil.h"
#include "../RenderComponent/MObject.h"

class UploadBuffer : public MObject
{
public:
	void Create(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, size_t stride, bool isUAV);
	UploadBuffer() : MObject() {}
    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ID3D12Resource* Resource()const
    {
        return mDefaultBuffer.Get();
    }
	virtual ~UploadBuffer();
	void CopyData(UINT elementIndex, const void* data);
	void CopyDatas(UINT startElementIndex, UINT elementCount, const void* data);
	size_t GetStride() const { return mStride; }
	size_t GetAlignedStride() const { return mElementByteSize; }
	static void UploadData(ID3D12GraphicsCommandList* commandList);
private:
	struct UploadCommand
	{
		UINT startIndex;
		UINT count;
	};
	bool GetUAV() const { return mIsUAV; }
	void SetUAV(bool isUAV, ID3D12GraphicsCommandList* cmdList);
	void UploadDataToDefaultBuffer(ID3D12GraphicsCommandList* commandList);
	static std::vector<UploadBuffer*> needUpdateLists;
	std::vector<UploadCommand> needUpdateElements;
	void* mMappedData;
	bool mIsUAV;
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDefaultBuffer;
	size_t mStride;
	UINT mElementCount;
    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
	bool isDirty = false;
};