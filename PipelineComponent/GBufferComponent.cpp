#include "GBufferComponent.h"
#include "PrepareComponent.h"
#include "../Common/d3dUtil.h"
#include "../Singleton/ShaderCompiler.h"
class GBufferRunnable
{
public:
	PipelineComponent::EventData data;
	ThreadCommand* tcmd;

	void operator()()
	{
		tcmd->ResetCommand();
		ID3D12GraphicsCommandList* commandList = tcmd->GetCmdList();
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(data.backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		commandList->ClearRenderTargetView(data.backBufferHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(data.backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		tcmd->CloseCommand();
	}
};
#include "../Common/GeometryGenerator.h"
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

JobHandle GBufferComponent::RenderEvent(EventData& data, JobBucket& taskFlow, ThreadCommand* commandList)
{
	GBufferRunnable runnable
	{
		data,
		commandList
	};
	JobHandle tsk = taskFlow.GetTask(runnable);
	return tsk;
}
std::vector<std::string> GBufferComponent::GetDependedEvent()
{
	std::vector<std::string> depending = {};
	return depending;
}
/*
GBufferComponent::GBufferComponent()
{
	new Texture(mCommandList.Get(), md3dDevice.Get(), "woodCrateTex", L"Textures/WoodCrate01.dds");
}*/