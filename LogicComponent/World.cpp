#include "World.h"
#include "../Common/GeometryGenerator.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/Material.h"
#include "../RenderComponent/Mesh.h"
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
World::World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device)
{

}

void World::Update(FrameResource* resource)
{
	
}