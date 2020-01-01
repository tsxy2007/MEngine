#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
struct StructuredBufferElement
{
	size_t stride;
	size_t elementCount;
};
class StructuredBuffer : public MObject
{
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mDefaultBuffer;
	std::vector<StructuredBufferElement> elements;
	std::vector<size_t> offsets;
	size_t GetStride(UINT index);
	size_t GetElementCount(UINT index);
	D3D12_GPU_VIRTUAL_ADDRESS GetAddress(UINT element, UINT index);
public:
	StructuredBuffer(
		ID3D12Device* device,
		StructuredBufferElement* elementsArray,
		UINT elementsCount
	);

};