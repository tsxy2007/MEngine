#pragma once
#include "../RenderComponent/MeshRenderer.h"
//Only For Test!
class World
{
public:
	UINT windowWidth;
	UINT windowHeight;
	ObjectPtr<MeshRenderer> testBox;
	ObjectPtr<Mesh> boxMesh;
	ObjectPtr<Material> testMat;
	ObjectPtr<DescriptorHeap> heap;
	ObjectPtr<Texture> mainTexture;
	ObjectPtr<UploadBuffer> materialBuffer;
	World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device);
	void Update(FrameResource* resource);
};