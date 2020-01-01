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
void BuildShapeGeometry(GeometryGenerator::MeshData& box, ObjectPtr<Mesh>& bMesh, ID3D12GraphicsCommandList* commandList, ID3D12Device* device)
{
	std::vector<XMFLOAT3> positions(box.Vertices.size());
	std::vector<XMFLOAT3> normals(box.Vertices.size());
	std::vector<XMFLOAT2> uvs(box.Vertices.size());
	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		positions[i] = box.Vertices[i].Position;
		normals[i] = box.Vertices[i].Normal;
		uvs[i] = box.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = box.GetIndices16();
	std::vector<SubMesh> subMeshs(1);
	subMeshs[0] =
	{
		DXGI_FORMAT_R16_UINT,
		(int)indices.size(),
		(void*)indices.data()
	};
	bMesh = new Mesh(
		box.Vertices.size(),
		positions.data(),
		normals.data(),
		nullptr,
		nullptr,
		uvs.data(),
		nullptr,
		nullptr,
		nullptr,
		device,
		commandList,
		subMeshs.data(),
		subMeshs.size()
	);

}
void BuildTexture(ObjectPtr<Texture>& texs, ID3D12GraphicsCommandList* cmdList, ID3D12Device* device)
{
	texs = new Texture(cmdList, device, "woodCrateTex", L"Textures/WoodCrate01.dds");
}
#pragma endregion
World::World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device)
{

}

void World::Update(FrameResource* resource)
{
	
}