#pragma once
//#include "../RenderComponent/MeshRenderer.h"
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/BitArray.h"
class FrameResource;
class Transform;
class DescriptorHeap;
//Only For Test!
class World
{
private:
	static const UINT MAXIMUM_HEAP_COUNT = 10000;
	ObjectPtr<DescriptorHeap> globalDescriptorHeap;
	BitArray usedDescs;
	std::vector<UINT> unusedDescs;
public:
	std::vector<Transform*> allTransformsPtr;
	UINT windowWidth;
	UINT windowHeight;
	World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device);
	void Update(FrameResource* resource);
	constexpr DescriptorHeap* GetGlobalDescHeap() const
	{ return globalDescriptorHeap.operator->(); }
	UINT GetDescHeapIndexFromPool();
	void ReturnDescHeapIndexToPool(UINT targetIndex);
	void ForceCollectAllHeapIndex();
};