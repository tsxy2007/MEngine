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
#define ALBEDO_RT (component->GetTempRT(0))
#define SPECULAR_RT (component->GetTempRT(1))
#define NORMAL_RT  (component->GetTempRT(2))
#define EMISSION_RT  (component->GetTempRT(3))
#define MOTION_VECTOR_RT (component->GetTempRT(4))

ObjectPtr<Material> mat;
ObjectPtr<Mesh> mesh;
ObjectPtr<MeshRenderer> meshRenderer;
ObjectPtr<Transform> trans;
ObjectPtr<GRP_Renderer> grpRenderer;
PrepareComponent* prepareComp = nullptr;
class GBufferFrameResource : public IPipelineResource
{
public:
	DescriptorHeap renderTargetHeap;
	UploadBuffer ub;
	UploadBuffer cullBuffer;
	GBufferFrameResource(ID3D12Device* device)
	{
		cullBuffer.Create(device, 1, true, sizeof(GRP_Renderer::CullData));
		ub.Create(device, 2, true, sizeof(ObjectConstants));
		renderTargetHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 5, false);
		XMFLOAT4X4 mat = MathHelper::Identity4x4();
		XMMATRIX* vec = (XMMATRIX*)&mat;
		vec->r[3] = { 0, 0.5, 0, 1 };
		ub.CopyData(0, &mat);
		vec->r[3] = { 0, -0.5, 0, 1 };
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
	GBufferComponent* component;//Singleton Component
	World* world;				//Main Scene
	void operator()()
	{
		tcmd->ResetCommand();
		ID3D12GraphicsCommandList* commandList = tcmd->GetCmdList();
		GBufferFrameResource* frameRes = (GBufferFrameResource*)resource->GetResource(component, cam, [=]()->GBufferFrameResource*
		{
			return new GBufferFrameResource(device);
		});
		grpRenderer->UpdateFrame(resource, device);
		//Bind To RTV Heap
		ALBEDO_RT->BindRTVToHeap(device, &frameRes->renderTargetHeap, 0, 0);
		SPECULAR_RT->BindRTVToHeap(device, &frameRes->renderTargetHeap, 1, 0);
		NORMAL_RT->BindRTVToHeap(device, &frameRes->renderTargetHeap, 2, 0);
		EMISSION_RT->BindRTVToHeap(device, &frameRes->renderTargetHeap, 3, 0);
		MOTION_VECTOR_RT->BindRTVToHeap(device, &frameRes->renderTargetHeap, 4, 0);
		//Clear
		ALBEDO_RT->ClearRenderTarget(commandList, 0, true, false);
		SPECULAR_RT->ClearRenderTarget(commandList, 0, true, false);
		NORMAL_RT->ClearRenderTarget(commandList, 0, true, false);
		MOTION_VECTOR_RT->ClearRenderTarget(commandList, 0, true, false);
		EMISSION_RT->ClearRenderTarget(commandList, 0, false, true);
		D3D12_CPU_DESCRIPTOR_HANDLE handles[5];
		auto st = [&](UINT p)->void
		{
			handles[p] = frameRes->renderTargetHeap.hCPU(p);
		};
		InnerLoop<decltype(st), 5>(st);
		EMISSION_RT->SetViewport(commandList);
		//Depth Prepass
		//TODO
		commandList->OMSetRenderTargets(0, nullptr, true, &EMISSION_RT->GetDepthDescriptor(0));
		/*meshRenderer->Draw(
			1, commandList,
			device,
			&resource->cameraCBs[cam->GetInstanceID()],
			&frameRes->ub,
			1, depthPrepassContainer
		);*/
		ConstBufferElement cullEle;
		cullEle.buffer = &frameRes->cullBuffer;
		cullEle.element = 0;
		grpRenderer->DrawCommand(
			commandList,
			device,
			1, resource,
			resource->cameraCBs[cam->GetInstanceID()],
			cullEle,
			prepareComp->frustumPlanes,
			*(XMFLOAT3*)&prepareComp->frustumMinPos,
			*(XMFLOAT3*)&prepareComp->frustumMaxPos,
			depthPrepassContainer
		);
		//GBuffer Pass
		//TODO
		commandList->OMSetRenderTargets(5, handles, true, &EMISSION_RT->GetDepthDescriptor(0));
		/*meshRenderer->Draw(
			0, commandList,
			device,
			&resource->cameraCBs[cam->GetInstanceID()],
			&frameRes->ub,
			1, depthPrepassContainer
		);*/
		grpRenderer->DrawCommand(
			commandList,
			device,
			0, resource,
			resource->cameraCBs[cam->GetInstanceID()],
			cullEle,
			prepareComp->frustumPlanes,
			*(XMFLOAT3*)&prepareComp->frustumMinPos,
			*(XMFLOAT3*)&prepareComp->frustumMaxPos,
			gbufferContainer
		);
		tcmd->CloseCommand();
	}
};

std::vector<TemporalRTCommand>& GBufferComponent::SendRenderTextureRequire(EventData& evt) {
	for (int i = 0; i < tempRTRequire.size(); ++i)
	{
		tempRTRequire[i].descriptor.width = evt.width;
		tempRTRequire[i].descriptor.height = evt.height;
	}
	return tempRTRequire;
}
void GBufferComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{
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
	tempRTRequire.resize(5);
	TemporalRTCommand& albedoBuffer = tempRTRequire[0];
	albedoBuffer.type = TemporalRTCommand::Create;
	albedoBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture0");
	albedoBuffer.descriptor.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	albedoBuffer.descriptor.depthType = RenderTextureDepthSettings_None;
	albedoBuffer.descriptor.depthSlice = 1;
	albedoBuffer.descriptor.type = RenderTextureType::Tex2D;

	TemporalRTCommand& specularBuffer = tempRTRequire[1];
	specularBuffer.type = TemporalRTCommand::Create;
	specularBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture1");
	specularBuffer.descriptor.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	specularBuffer.descriptor.depthType = RenderTextureDepthSettings_None;
	specularBuffer.descriptor.depthSlice = 1;
	specularBuffer.descriptor.type = RenderTextureType::Tex2D;

	TemporalRTCommand& normalBuffer = tempRTRequire[2];
	normalBuffer.type = TemporalRTCommand::Create;
	normalBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture2");
	normalBuffer.descriptor.colorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	normalBuffer.descriptor.depthType = RenderTextureDepthSettings_None;
	normalBuffer.descriptor.depthSlice = 1;
	normalBuffer.descriptor.type = RenderTextureType::Tex2D;

	TemporalRTCommand& emissionBuffer = tempRTRequire[3];
	emissionBuffer.type = TemporalRTCommand::Create;
	emissionBuffer.uID = ShaderID::PropertyToID("_CameraRenderTarget");
	emissionBuffer.descriptor.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	emissionBuffer.descriptor.depthType = RenderTextureDepthSettings_Depth32;
	emissionBuffer.descriptor.depthSlice = 1;
	emissionBuffer.descriptor.type = RenderTextureType::Tex2D;

	TemporalRTCommand& motionVectorBuffer = tempRTRequire[4];
	motionVectorBuffer.type = TemporalRTCommand::Create;
	motionVectorBuffer.uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	motionVectorBuffer.descriptor.colorFormat = DXGI_FORMAT_R16G16_SNORM;
	motionVectorBuffer.descriptor.depthType = RenderTextureDepthSettings_None;
	motionVectorBuffer.descriptor.depthSlice = 1;
	motionVectorBuffer.descriptor.type = RenderTextureType::Tex2D;

	std::vector<DXGI_FORMAT> colorFormats(tempRTRequire.size());
	for (int i = 0; i < tempRTRequire.size(); ++i)
	{
		colorFormats[i] = tempRTRequire[i].descriptor.colorFormat;
	}
	gbufferContainer = new PSOContainer(DXGI_FORMAT_D32_FLOAT, colorFormats.size(), colorFormats.data());
	depthPrepassContainer = new PSOContainer(DXGI_FORMAT_D32_FLOAT, 0, nullptr);
	SetCPUDepending<PrepareComponent>();
	ObjectPtr<UploadBuffer> materialPropertyBuffer = new UploadBuffer();
	materialPropertyBuffer->Create(device, 1, true, sizeof(MaterialConstants));
	ObjectPtr<DescriptorHeap> heap;
	mat = new Material(ShaderCompiler::GetShader("OpaqueStandard"), materialPropertyBuffer, 0, heap);
	GeometryGenerator geoGen;
	BuildShapeGeometry(geoGen.CreateBox(1, 1, 1, 1), mesh, device, commandList, nullptr);
	trans = new Transform(nullptr);
	meshRenderer = new MeshRenderer(trans.operator->(), device, mesh, mat);
	grpRenderer = new GRP_Renderer(
		sizeof(MaterialConstants),
		MeshLayout::GetMeshLayoutIndex(
			true,
			false,
			false,
			true,
			false,
			false,
			false,
			false
		),
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