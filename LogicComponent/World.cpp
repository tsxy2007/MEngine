#include "World.h"
#include "../Common/GeometryGenerator.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/Material.h"
#include "../RenderComponent/Mesh.h"
#include "../RenderComponent/DescriptorHeap.h"
using namespace DirectX;
#pragma region  TEST_BUILDING_AREA
void BuildMaterials(ObjectPtr<UploadBuffer>& materialPropertyBuffer, ID3D12Device* device, ObjectPtr<Material>& opaqueMaterial, ObjectPtr<DescriptorHeap>& bindlessTextureHeap)
{
	materialPropertyBuffer = new UploadBuffer();
	materialPropertyBuffer->Create(device, 1, true, sizeof(MaterialConstants));
	opaqueMaterial = new Material(ShaderCompiler::GetShader("OpaqueStandard"), materialPropertyBuffer, 0, bindlessTextureHeap);
	//opaqueMaterial->SetTexture2D(ShaderID::PropertyToID("gDiffuseMap"), mTextures[0]);
	opaqueMaterial->SetBindlessResource(ShaderID::PropertyToID("gDiffuseMap"), 0);
	opaqueMaterial->SetBindlessResource(ShaderID::PropertyToID("cubemap"), 9);

}
#pragma endregion
World::World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device) :
	usedDescs(MAXIMUM_HEAP_COUNT),
	unusedDescs(MAXIMUM_HEAP_COUNT)
{
	for (UINT i = 0; i < MAXIMUM_HEAP_COUNT; ++i)
	{
		unusedDescs[i] = i;
	}
}

UINT World::GetDescHeapIndexFromPool()
{
	auto&& last = unusedDescs.end() - 1;
	UINT value = *last;
	unusedDescs.erase(last);
	usedDescs[value] = true;
	return value;
}

void World::ReturnDescHeapIndexToPool(UINT target)
{
	auto ite = usedDescs[target];
	if (ite)
	{
		unusedDescs.push_back(target);
		ite = false;
	}
}

void World::ForceCollectAllHeapIndex()
{
	for (UINT i = 0; i < MAXIMUM_HEAP_COUNT; ++i)
	{
		auto ite = usedDescs[i];
		if (ite)
		{
			unusedDescs.push_back(i);
			ite = false;
		}
	}
}

void World::Update(FrameResource* resource)
{

}