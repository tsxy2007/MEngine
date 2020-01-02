#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
class UploadBuffer;
struct ConstBufferElement
{
	ObjectPtr<UploadBuffer> buffer;
	UINT element;
};
class CBufferPool
{
private:
	struct PoolValue
	{
		ObjectPtr<UploadBuffer> buffer;
		std::vector<UINT>* pool;
	};
	UINT mStride;
	UINT initElementCount;
	std::vector<PoolValue> allPoolValues;
	std::unordered_map<UploadBuffer*, UINT> allPoolKeys;
public:
	CBufferPool(UINT stride, UINT initCapacity);
	ConstBufferElement GetBuffer(ID3D12Device* device);
	void Release(ConstBufferElement element);
	virtual ~CBufferPool();
};