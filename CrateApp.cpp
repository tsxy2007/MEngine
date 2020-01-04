//***************************************************************************************
// CrateApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************
#include "RenderComponent/Shader.h"
#include "RenderComponent/Material.h"
#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "RenderComponent/UploadBuffer.h"
#include "Common/GeometryGenerator.h"
#include "Singleton/FrameResource.h"
#include "Singleton/ShaderID.h"
#include "RenderComponent/Texture.h"
#include "Singleton/MeshLayout.h"
#include "Singleton/PSOContainer.h"
#include "Common/Camera.h"
#include "RenderComponent/Mesh.h"
#include "RenderComponent/MeshRenderer.h"
#include "Singleton/ShaderCompiler.h"
#include "RenderComponent/Skybox.h"
#include "RenderComponent/ComputeShader.h"
#include "RenderComponent/IndirectDrawer.h"
#include "RenderComponent/RenderTexture.h"
#include "PipelineComponent/RenderPipeline.h"
#include "Singleton/Graphics.h"
#include "Singleton/MathLib.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;
	Mesh* mesh = nullptr;
	FMaterial* Mat = nullptr;
	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};
#include "LogicComponent/World.h"
class CrateApp : public D3DApp
{
public:
	typedef std::aligned_storage_t<sizeof(World), alignof(World)> WorldStorage;
	CrateApp(HINSTANCE hInstance);
	CrateApp(const CrateApp& rhs) = delete;
	CrateApp& operator=(const CrateApp& rhs) = delete;
	~CrateApp();

	virtual bool Initialize()override;
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	//void AnimateMaterials(const GameTimer& gt);
	//void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);

	void LoadTextures();
	void BuildDescriptorHeaps();
	void BuildShapeGeometry(GeometryGenerator::MeshData& box, ObjectPtr<Mesh>& bMesh);
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	//void BuildRenderItems();
	//void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	bool lastFrameExecute = false;
	int mCurrFrameResourceIndex = 0;
	//ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
//	ObjectPtr<RenderTexture> mainRenderTexture;
//	ObjectPtr<DescriptorHeap> bindlessTextureHeap;
//	ObjectPtr<DescriptorHeap> uavTextureHeap;
//	ObjectPtr<IndirectDrawer> indirectDrawer;
	RenderPipeline* rp;
	//	std::unordered_map<std::string, std::unique_ptr<FMaterial>> mMaterials;
	//	std::vector<ObjectPtr<Texture>> mTextures;
	//	ObjectPtr<Mesh> boxMesh;
	//	ObjectPtr<Mesh> sphereMesh;
		//	Shader* opaqueShader;
		//ObjectPtr<Material> opaqueMaterial;
	//	ObjectPtr<MeshRenderer> mainRenderer;
	//	ObjectPtr<MeshRenderer> mainRenderer1;
		//ObjectPtr<Skybox> skybox;
		//PassConstants mMainPassCB;
	ObjectPtr<Camera> mainCamera;

	//ObjectPtr <UploadBuffer> materialPropertyBuffer;
	float mTheta = 1.3f*XM_PI;
	float mPhi = 0.4f*XM_PI;
	float mRadius = 2.5f;
	POINT mLastMousePos;
	//ThreadCommand* mainThreadCommand;
	//ThreadCommand* separateThreadCommand;

	//StoragePSOContainer mainRTPsoContainer;
	//StoragePSOContainer mirrorRTPsoContainter;
	//StorageComputeShader testComputeShader;
	FrameResource* lastResource = nullptr;
	std::unique_ptr<ThreadCommand> directThreadCommand;
	WorldStorage worldPtr;
};
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		CrateApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

CrateApp::CrateApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
	JobSystem::Initialize(10);
}

CrateApp::~CrateApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
	JobSystem::Dispose();
	RenderPipeline::DestroyInstance();
	((World*)&worldPtr)->~World();
}

bool CrateApp::Initialize()
{

	if (!D3DApp::Initialize())
		return false;
	ShaderID::Init();
	directThreadCommand = std::make_unique<ThreadCommand, ID3D12Device*>(md3dDevice.Get());
	directThreadCommand->ResetCommand();
	// Reset the command list to prep for initialization commands.
	ShaderCompiler::Init(md3dDevice.Get());
	LoadTextures();
	BuildDescriptorHeaps();
	GeometryGenerator geoGen;
	//BuildShapeGeometry(geoGen.CreateBox(1, 1, 1, 1), boxMesh);
	//BuildShapeGeometry(geoGen.CreateSphere(1, 32, 33), sphereMesh);
	//BuildMaterials();
	BuildFrameResources();
	BuildPSOs();
	mainCamera = new Camera(md3dDevice.Get(), Camera::CameraRenderPath::DefaultPipeline);
	//BuildPSOs();
	// Execute the initialization commands.
	// Wait until initialization is complete.
	new (&worldPtr)World(directThreadCommand->GetCmdList(), md3dDevice.Get());
	rp = RenderPipeline::GetInstance(md3dDevice.Get(),
		directThreadCommand->GetCmdList());
	Graphics::Initialize(md3dDevice.Get(), directThreadCommand->GetCmdList());
	directThreadCommand->CloseCommand();
	ID3D12CommandList* lst = directThreadCommand->GetCmdList();
	mCommandQueue->ExecuteCommandLists(1, &lst);
	FlushCommandQueue();
	directThreadCommand = nullptr;
	return true;
}

void CrateApp::OnResize()
{
	D3DApp::OnResize();
	lastFrameExecute = false;
}

void CrateApp::Update(const GameTimer& gt)
{
	if (mClientHeight < 1 || mClientWidth < 1) return;
	OnKeyboardInput(gt);
	UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	lastResource = FrameResource::mCurrFrameResource;
	FrameResource::mCurrFrameResource = FrameResource::mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	FrameResource::mCurrFrameResource->UpdateBeforeFrame(mFence.Get());
	UpdateMaterialCBs(gt);
	mainCamera->SetLens(0.333333 * MathHelper::Pi, AspectRatio(), 1.5, 100);
	((World*)&worldPtr)->Update(FrameResource::mCurrFrameResource);
	//mainRenderer->UpdateObjectBuffer(FrameResource::mCurrFrameResource);
	//mainRenderer1->UpdateObjectBuffer(FrameResource::mCurrFrameResource);
	//mainCamera->UploadCameraBuffer(FrameResource::mCurrFrameResource, mMainPassCB);
	/*MaterialConstants matConstants;
	matConstants.DiffuseAlbedo = { 1,1,1,1 };
	matConstants.FresnelR0 = { 1,1,1 };
	matConstants.Roughness = 1;
	ObjectConstants objCsts[2];
	XMStoreFloat4x4(&objCsts[0].objectToWorld, mainRenderer->transform.GetLocalToWorldMatrix());
	XMStoreFloat4x4(&objCsts[1].objectToWorld, mainRenderer1->transform.GetLocalToWorldMatrix());
	indirectDrawer->UploadMaterialBuffer(&matConstants, 0);
	indirectDrawer->UploadObjectBuffer(objCsts, 0);
	indirectDrawer->UploadObjectBuffer(objCsts + 1, 1);*/
}
std::vector<Camera*> cam(1);
void CrateApp::Draw(const GameTimer& gt)
{
	if (mClientHeight < 1 || mClientWidth < 1) return;
	/*mainThreadCommand = FrameResource::mCurrFrameResource->GetNewThreadCommand(mainCamera.operator->(), md3dDevice.Get());
	separateThreadCommand = FrameResource::mCurrFrameResource->GetNewThreadCommand(mainCamera.operator->(), md3dDevice.Get());
	FrameResource::mCurrFrameResource->ReleaseThreadCommand(mainCamera.operator->(), mainThreadCommand);
	FrameResource::mCurrFrameResource->ReleaseThreadCommand(mainCamera.operator->(), separateThreadCommand);
	ID3D12GraphicsCommandList* mCommandList = mainThreadCommand->GetCmdList();
	taskFlow.clear();
	ConstBufferElement* camBuffer = &FrameResource::mCurrFrameResource->cameraCBs[mainCamera->GetInstanceID()];
	PSOContainer* separatePSO = (PSOContainer*)&mirrorRTPsoContainter;
	DrawRenderTextureCommand func = { this, camBuffer, separatePSO };
	tf::Task firstTask = taskFlow.emplace(func);
	std::future<void> runner = taskFlowExecutor.run(taskFlow);
	mainThreadCommand->ResetCommand();
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	// Clear the back buffer and depth buffer.

	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	PSOContainer* mainPSO = (PSOContainer*)&mainRTPsoContainer;
	skybox->Draw(
		0,
		mCommandList,
		md3dDevice.Get(),
		camBuffer,
		FrameResource::mCurrFrameResource,
		mainPSO
	);
	mainRenderer->Draw(
		0,
		0,
		mCommandList,
		md3dDevice.Get(),
		camBuffer,
		FrameResource::mCurrFrameResource,
		mainPSO
	);
	mainRenderer1->Draw(
		0,
		0,
		mCommandList,
		md3dDevice.Get(),
		camBuffer,
		FrameResource::mCurrFrameResource,
		mainPSO
	);
	IndirectDrawer::HeapSet hpSt;
	hpSt.heapOffset = 0;
	hpSt.shaderID = ShaderID::PropertyToID("gDiffuseMap");
	indirectDrawer->Draw(
		0,
		mCommandList,
		md3dDevice.Get(),
		camBuffer,
		FrameResource::mCurrFrameResource,
		mainPSO,
		bindlessTextureHeap.operator->(),
		&hpSt,
		1
	);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	mCommandList->Close();
	mCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)mCommandList.GetAddressOf());*/
	cam[0] = mainCamera.operator->();
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
	RenderPipelineData data;
	data.device = md3dDevice.Get();
	data.backBufferHandle = CurrentBackBufferView();
	data.backBufferResource = CurrentBackBuffer();
	data.commandQueue = mCommandQueue.Get();
	data.lastResource = lastResource;
	data.resource = FrameResource::mCurrFrameResource;
	data.allCameras = &cam;
	data.fence = mFence.Get();
	data.fenceIndex = &mCurrentFence;
	data.executeLastFrame = lastFrameExecute;
	data.swap = mSwapChain.Get();
	data.world = (World*)&worldPtr;
	data.world->windowWidth = mClientWidth;
	data.world->windowHeight = mClientHeight;
	rp->RenderCamera(data);

	lastFrameExecute = true;
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CrateApp::OnKeyboardInput(const GameTimer& gt)
{
}

void CrateApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	XMFLOAT3 mEyePos;
	mEyePos.x = mRadius * sinf(mPhi)*cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi)*sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);
	XMFLOAT3 target = { 0,0,0 };
	XMFLOAT3 up = { 0,1,0 };
	mainCamera->LookAt(mEyePos, target, up);

}

void CrateApp::UpdateMaterialCBs(const GameTimer& gt)
{/*
	auto currMaterialCB = materialPropertyBuffer;
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		FMaterial* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, (matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, &matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
	*/
}

void CrateApp::LoadTextures()
{
	/*	mTextures.reserve(12);
		mTextures.clear();
		auto woodCrateTex = new Texture(mCommandList.Get(), md3dDevice.Get(), "woodCrateTex", L"Textures/WoodCrate01.dds");
		mTextures.push_back(woodCrateTex);
		auto brickTex = new Texture(mCommandList.Get(), md3dDevice.Get(), "brickTex", L"Textures/bricks.dds");
		mTextures.push_back(brickTex);
		brickTex = new Texture(mCommandList.Get(), md3dDevice.Get(), "brickTex2", L"Textures/bricks2.dds");
		mTextures.push_back(brickTex);
		brickTex = new Texture(mCommandList.Get(), md3dDevice.Get(), "brickTex3", L"Textures/bricks3.dds");
		mTextures.push_back(brickTex);
		brickTex = new Texture(mCommandList.Get(), md3dDevice.Get(), "grass", L"Textures/grass.dds");
		mTextures.push_back(brickTex);
		brickTex = new Texture(mCommandList.Get(), md3dDevice.Get(), "head_diff", L"Textures/head_diff.dds");
		mTextures.push_back(brickTex);
		brickTex = new Texture(mCommandList.Get(), md3dDevice.Get(), "ice", L"Textures/ice.dds");
		mTextures.push_back(brickTex);
		brickTex = new Texture(mCommandList.Get(), md3dDevice.Get(), "jacket_diff", L"Textures/jacket_diff.dds");
		mTextures.push_back(brickTex);
		brickTex = new Texture(mCommandList.Get(), md3dDevice.Get(), "jacket_diff", L"Textures/pants_diff.dds");
		mTextures.push_back(brickTex);
		brickTex = new Texture(mCommandList.Get(), md3dDevice.Get(), "grasscube1024", L"Textures/grasscube1024.dds", Texture::Cubemap);
		mTextures.push_back(brickTex);*/
		//	mainRenderTexture = new RenderTexture(md3dDevice.Get(), 1024, 1024, DXGI_FORMAT_R16G16B16A16_FLOAT, true, RenderTextureType::Tex2D, 1, 2);
}

void CrateApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	/*
	bindlessTextureHeap = new DescriptorHeap();
	bindlessTextureHeap->Create(md3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mTextures.size() + 100, true);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	mainRenderTexture->GetColorViewDesc(srvDesc);
	md3dDevice->CreateShaderResourceView(mainRenderTexture->GetColorResource(), &srvDesc, bindlessTextureHeap->hCPU(0));
	for (int i = 0; i < mTextures.size(); ++i)
	{
		mTextures[i]->GetResourceViewDescriptor(srvDesc);
		md3dDevice->CreateShaderResourceView(mTextures[i]->GetResource(), &srvDesc, bindlessTextureHeap->hCPU(i + 1));
	}
	uavTextureHeap = new DescriptorHeap();
	uavTextureHeap->Create(md3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	mainRenderTexture->GetColorUAVDesc(uavDesc, 0);
	md3dDevice->CreateUnorderedAccessView(mainRenderTexture->GetColorResource(), nullptr, &uavDesc, uavTextureHeap->hCPU(0));
	mainRenderTexture->GetColorUAVDesc(uavDesc, 1);
	md3dDevice->CreateUnorderedAccessView(mainRenderTexture->GetColorResource(), nullptr, &uavDesc, uavTextureHeap->hCPU(1));*/
}

void CrateApp::BuildShapeGeometry(GeometryGenerator::MeshData& box, ObjectPtr<Mesh>& bMesh)
{
	/*std::vector<XMFLOAT3> positions(box.Vertices.size());
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
		md3dDevice.Get(),
		mCommandList.Get(),
		subMeshs
	);
	*/
}
void CrateApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		FrameResource::mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, 1));
	}
}

void CrateApp::BuildMaterials()
{
	/*
	auto woodCrate = std::make_unique<FMaterial>();
	woodCrate->Name = "woodCrate";
	woodCrate->MatCBIndex = 0;
	woodCrate->DiffuseSrvHeapIndex = 0;
	woodCrate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	woodCrate->Roughness = 0.2f;

	mMaterials["woodCrate"] = std::move(woodCrate);
	materialPropertyBuffer = new UploadBuffer();
	materialPropertyBuffer->Create(md3dDevice.Get(), 1, true, sizeof(MaterialConstants), false);
	opaqueMaterial = new Material(ShaderCompiler::GetShader("OpaqueStandard"), materialPropertyBuffer, 0, bindlessTextureHeap);
	//opaqueMaterial->SetTexture2D(ShaderID::PropertyToID("gDiffuseMap"), mTextures[0]);

	opaqueMaterial->SetBindlessResource(ShaderID::PropertyToID("gDiffuseMap"), 0);
	opaqueMaterial->SetBindlessResource(ShaderID::PropertyToID("cubemap"), 9);
	skybox = new Skybox(mTextures[9], md3dDevice.Get(), mCommandList.Get());
	*/
}

void CrateApp::BuildPSOs()
{
	/*
	DXGI_FORMAT colorFormat = mainRenderTexture->GetColorFormat();
	new (&mirrorRTPsoContainter)PSOContainer(mainRenderTexture->GetDepthFormat(), 1, &colorFormat);
	new(&mainRTPsoContainer)PSOContainer(mainRenderTexture->GetDepthFormat(), 1, &mBackBufferFormat);
	std::vector<std::string> kernelNames(1);
	kernelNames[0] = "CSMain";
	std::vector<ComputeShaderVariable> vars(1);
	vars[0].space = 0;
	vars[0].registerPos = 0;
	vars[0].type = ComputeShaderVariable::UAVDescriptorHeap;
	vars[0].tableSize = 2;
	vars[0].name = "_MainTex";
	new(&testComputeShader)ComputeShader(L"Shaders\\ComputeShaderTest.hlsl", kernelNames, vars, md3dDevice.Get());
	std::array<MeshCommand, 2> comds;
	comds[0].materialIndex = 0;
	comds[0].mesh = boxMesh.operator->();
	comds[0].subMeshIndex = 0;
	comds[1].materialIndex = 0;
	comds[1].mesh = sphereMesh.operator->();
	comds[1].subMeshIndex = 0;
	indirectDrawer = new IndirectDrawer(
		ShaderCompiler::GetShader("OpaqueStandard"),
		comds.data(),
		2,
		sizeof(ObjectConstants),
		sizeof(MaterialConstants),
		1,
		md3dDevice.Get(),
		mCommandList.Get()
	);*/
}