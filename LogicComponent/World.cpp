#include "World.h"
#include "../Common/GeometryGenerator.h"
#include "../Common/d3dUtil.h"
#include "../Singleton/ShaderCompiler.h"
using namespace DirectX;
#pragma region  TEST_BUILDING_AREA
void BuildMaterials(ObjectPtr<UploadBuffer>& materialPropertyBuffer, ID3D12Device* device, ObjectPtr<Material>& opaqueMaterial, ObjectPtr<DescriptorHeap>& bindlessTextureHeap)
{
	materialPropertyBuffer = new UploadBuffer();
	materialPropertyBuffer->Create(device, 1, true, sizeof(MaterialConstants), false);
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
	std::vector<SubMesh>* subMeshs = new std::vector<SubMesh>(1);
	(*subMeshs)[0] =
	{
		DXGI_FORMAT_R16_UINT,
		(int)indices.size(),
		(void*)indices.data(),
		{0,0,0},
		{1,1,1}
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
		subMeshs
	);

}
void BuildTexture(ObjectPtr<Texture>& texs, ID3D12GraphicsCommandList* cmdList, ID3D12Device* device)
{
	texs = new Texture(cmdList, device, "woodCrateTex", L"Textures/WoodCrate01.dds");
}
#pragma endregion
World::World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device)
{
	GeometryGenerator geoGen;
	heap = new DescriptorHeap();
	heap->Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);
	BuildShapeGeometry(geoGen.CreateBox(1, 1, 1, 1), boxMesh, cmdList, device);
	BuildMaterials(materialBuffer, device, testMat, heap);
	BuildTexture(mainTexture, cmdList, device);
	DirectX::XMFLOAT3 pos = { 0,0,0 };
	DirectX::XMVECTOR rotation = { 0,0,0,1 };
	DirectX::XMFLOAT3 scale = { 1,1,1 };
	std::vector<ObjectPtr<Material>> mats = { testMat };
	testBox = new MeshRenderer(
		device,
		pos,
		rotation,
		scale,
		boxMesh,
		mats
	);
}

void World::Update(FrameResource* resource)
{
	testBox->UpdateObjectBuffer(resource);
}