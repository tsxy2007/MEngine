#pragma once
//#include "../RenderComponent/MeshRenderer.h"
#include "../Common/d3dUtil.h"
class FrameResource;
//Only For Test!
class World
{
public:
	UINT windowWidth;
	UINT windowHeight;
	World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device);
	void Update(FrameResource* resource);
};