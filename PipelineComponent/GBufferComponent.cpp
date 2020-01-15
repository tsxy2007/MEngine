#include "GBufferComponent.h"
#include "../Common/d3dUtil.h"
#include "../Singleton/Graphics.h"
#include "../LogicComponent/World.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../Singleton/PSOContainer.h"
#include "../RenderComponent/StructuredBuffer.h"
#include "../RenderComponent/MeshRenderer.h"
#include "RenderPipeline.h"
#include "../LogicComponent/Transform.h"
#include "../Common/GeometryGenerator.h"
#include "../RenderComponent/GPU Driven/GRP_Renderer.h"
#include "../Singleton/MeshLayout.h"
#include "PrepareComponent.h"
using namespace DirectX;
PSOContainer* gbufferContainer(nullptr);
PSOContainer* depthPrepassContainer(nullptr);
RenderTexture* gbufferTempRT[10];
#define ALBEDO_RT (gbufferTempRT[0])
#define SPECULAR_RT (gbufferTempRT[1])
#define NORMAL_RT  (gbufferTempRT[2])
#define EMISSION_RT  (gbufferTempRT[3])
#define MOTION_VECTOR_RT (gbufferTempRT[4])
#define DEPTH_RT (gbufferTempRT[5])

ObjectPtr<Material> mat;
ObjectPtr<Mesh> mesh;
ObjectPtr<MeshRenderer> meshRenderer;
ObjectPtr<Transform> trans;
ObjectPtr<GRP_Renderer> grpRenderer;
PrepareComponent* prepareComp = nullptr;
class GBufferFrameResource : public IPipelineResource
{
public:
	UploadBuffer ub;
	UploadBuffer cullBuffer;
	GBufferFrameResource(ID3D12Device* device)
	{
		cullBuffer.Create(device, 1, true, sizeof(GRP_Renderer::CullData));
		ub.Create(device, 2, true, sizeof(ObjectConstants));
		XMFLOAT4X4 mat = MathHelper::Identity4x4();
		XMMATRIX* vec = (XMMATRIX*)&mat;
		vec->r[3] = { 0, 0, 0, 1 };
		ub.CopyData(0, &mat);
		vec->r[3] = { 0, 0, 0, 1 };
		ub.CopyData(1, &mat);
	}
};
class GBufferRunnable
{
public:
	ID3D12Device* device;		//Dx12 Device
	ThreadCommand* tcmd;		//Command List
	Camera* cam;				//Camera
	FrameResource* resource;	//Per Frame Data
	void* component;//Singleton Component
	World* world;				//Main Scene
	void operator()()
	{
		tcmd->ResetCommand();
		ID3D12GraphicsCommandList* commandList = tcmd->GetCmdList();
		GBufferFrameResource* frameRes = (GBufferFrameResource*)resource->GetPerCameraResource(component, cam, [=]()->GBufferFrameResource*
		{
			return new GBufferFrameResource(device);
		});
		grpRenderer->UpdateFrame(resource, device);
		//Clear
		ALBEDO_RT->ClearRenderTarget(commandList, 0);
		SPECULAR_RT->ClearRenderTarget(commandList, 0);
		NORMAL_RT->ClearRenderTarget(commandList, 0);
		MOTION_VECTOR_RT->ClearRenderTarget(commandList, 0);
		EMISSION_RT->ClearRenderTarget(commandList, 0);
		DEPTH_RT->ClearRenderTarget(commandList, 0);

		D3D12_CPU_DESCRIPTOR_HANDLE handles[5];
		auto st = [&](UINT p)->void
		{
			handles[p] = gbufferTempRT[p]->GetColorDescriptor(p);
		};
		InnerLoop<decltype(st), 5>(st);
		EMISSION_RT->SetViewport(commandList);
		ConstBufferElement cullEle;
		cullEle.buffer = &frameRes->cullBuffer;
		cullEle.element = 0;
		grpRenderer->Culling(
			commandList,
			device,
			resource,
			cullEle,
			prepareComp->frustumPlanes,
			*(XMFLOAT3*)&prepareComp->frustumMinPos,
			*(XMFLOAT3*)&prepareComp->frustumMaxPos
		);
		//Depth Prepass
		//TODO
		commandList->OMSetRenderTargets(0, nullptr, true, &DEPTH_RT->GetColorDescriptor(0));
	/*	meshRenderer->Draw(
			1, commandList,
			device,
			&resource->cameraCBs[cam->GetInstanceID()],
			&frameRes->ub,
			1, depthPrepassContainer
		);*/

		grpRenderer->DrawCommand(
			commandList,
			device,
			1,
			resource->cameraCBs[cam->GetInstanceID()],
			depthPrepassContainer
		);
		//GBuffer Pass
		//TODO
		commandList->OMSetRenderTargets(5, handles, false, &DEPTH_RT->GetColorDescriptor(0));
		/*	meshRenderer->Draw(
			0, commandList,
			device,
			&resource->cameraCBs[cam->GetInstanceID()],
			&frameRes->ub,
			1, gbufferContainer
		);*/
			grpRenderer->DrawCommand(
				commandList,
				device,
				0,
				resource->cameraCBs[cam->GetInstanceID()],
				gbufferContainer
			);
		tcmd->CloseCommand();
	}
};

std::vector<TemporalResourceCommand>& GBufferComponent::SendRenderTextureRequire(EventData& evt) {
	for (int i = 0; i < tempRTRequire.size(); ++i)
	{
		tempRTRequire[i].descriptor.rtDesc.width = evt.width;
		tempRTRequire[i].descriptor.rtDesc.height = evt.height;
	}
	return tempRTRequire;
}
void GBufferComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
	memcpy(gbufferTempRT, allTempResource.data(), sizeof(RenderTexture*) * tempRTRequire.size());
	GBufferRunnable runnable
	{
		data.device,
		commandList,
		data.camera,
		data.resource,
		this,
		data.world
	};
	//Schedule MultiThread Job
	ScheduleJob(runnable);
}

void BuildShapeGeometry(GeometryGenerator::MeshData& box, ObjectPtr<Mesh>& bMesh, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, FrameResource* res);

void GBufferComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<PrepareComponent>();
	tempRTRequire.resize(6);
	TemporalResourceCommand& albedoBuffer = tempRTRequire[0];
	albedoBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	albedoBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture0");
	albedoBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	albedoBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	albedoBuffer.descriptor.rtDesc.depthSlice = 1;
	albedoBuffer.descriptor.rtDesc.type = RenderTextureType::RenderTextureType_Tex2D;

	TemporalResourceCommand& specularBuffer = tempRTRequire[1];
	specularBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	specularBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture1");
	specularBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	specularBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	specularBuffer.descriptor.rtDesc.depthSlice = 1;
	specularBuffer.descriptor.rtDesc.type = RenderTextureType::RenderTextureType_Tex2D;

	TemporalResourceCommand& normalBuffer = tempRTRequire[2];
	normalBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	normalBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture2");
	normalBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	normalBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	normalBuffer.descriptor.rtDesc.depthSlice = 1;
	normalBuffer.descriptor.rtDesc.type = RenderTextureType::RenderTextureType_Tex2D;

	TemporalResourceCommand& emissionBuffer = tempRTRequire[3];
	emissionBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	emissionBuffer.uID = ShaderID::PropertyToID("_CameraRenderTarget");
	emissionBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	emissionBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	emissionBuffer.descriptor.rtDesc.depthSlice = 1;
	emissionBuffer.descriptor.rtDesc.type = RenderTextureType::RenderTextureType_Tex2D;

	TemporalResourceCommand& motionVectorBuffer = tempRTRequire[4];
	motionVectorBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	motionVectorBuffer.uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	motionVectorBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16_SNORM;
	motionVectorBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_ColorBuffer;
	motionVectorBuffer.descriptor.rtDesc.depthSlice = 1;
	motionVectorBuffer.descriptor.rtDesc.type = RenderTextureType::RenderTextureType_Tex2D;

	TemporalResourceCommand& depthBuffer = tempRTRequire[5];
	depthBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	depthBuffer.uID = ShaderID::PropertyToID("_CameraDepthTexture");
	depthBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::RenderTextureUsage_DepthBuffer;
	depthBuffer.descriptor.rtDesc.rtFormat.depthFormat = RenderTextureDepthSettings_Depth32;
	depthBuffer.descriptor.rtDesc.depthSlice = 1;
	depthBuffer.descriptor.rtDesc.type = RenderTextureType::RenderTextureType_Tex2D;

	std::vector<DXGI_FORMAT> colorFormats(tempRTRequire.size());
	for (int i = 0; i < 5; ++i)
	{
		colorFormats[i] = tempRTRequire[i].descriptor.rtDesc.rtFormat.colorFormat;
	}
	gbufferContainer = new PSOContainer(DXGI_FORMAT_D32_FLOAT, colorFormats.size(), colorFormats.data());
	depthPrepassContainer = new PSOContainer(DXGI_FORMAT_D32_FLOAT, 0, nullptr);
	ObjectPtr<UploadBuffer> materialPropertyBuffer = new UploadBuffer();
	materialPropertyBuffer->Create(device, 1, true, sizeof(MaterialConstants));
	ObjectPtr<DescriptorHeap> heap;
	mat = new Material(ShaderCompiler::GetShader("OpaqueStandard"), materialPropertyBuffer, 0, heap);
	GeometryGenerator geoGen;
	mesh = Mesh::LoadMeshFromFile(L"Wheel.vmesh", device);
	if (!mesh)
	{
		BuildShapeGeometry(geoGen.CreateBox(1, 1, 1, 1), mesh, device, commandList, nullptr);
	}
	trans = new Transform(nullptr);
	meshRenderer = new MeshRenderer(trans.operator->(), device, mesh, mat);
	grpRenderer = new GRP_Renderer(
		sizeof(MaterialConstants),
		mesh->GetLayoutIndex(),
		256,
		10000,
		ShaderCompiler::GetShader("OpaqueStandard"),
		2,
		device
	);
	grpRenderer->AddRenderElement(
		trans, mesh, device
	);
	prepareComp = RenderPipeline::GetComponent<PrepareComponent>();
}

void GBufferComponent::Dispose()
{

	//meshRenderer->Destroy();
	trans->Destroy();
	delete gbufferContainer;
	delete depthPrepassContainer;
}

void BuildShapeGeometry(GeometryGenerator::MeshData& box, ObjectPtr<Mesh>& bMesh, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, FrameResource* res)
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
		DXGI_FORMAT_R16_UINT,
		indices.size(),
		indices.data()
	);

}