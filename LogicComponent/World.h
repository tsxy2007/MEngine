#pragma once
//#include "../RenderComponent/MeshRenderer.h"
#include "../Common/d3dUtil.h"
class FrameResource;
class Transform;
//Only For Test!
class World
{
public:
	std::vector<Transform*> allTransformsPtr;
	UINT windowWidth;
	UINT windowHeight;
	World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device);
	void Update(FrameResource* resource);
};