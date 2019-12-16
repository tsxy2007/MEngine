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
typedef std::aligned_storage_t<sizeof(PSOContainer), alignof(PSOContainer)> StoragePSOContainer;
typedef std::aligned_storage_t<sizeof(ComputeShader), alignof(ComputeShader)> StorageComputeShader;
class CrateApp : public D3DApp
{
public:
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
	void AnimateMaterials(const GameTimer& gt);
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
	
	int mCurrFrameResourceIndex = 0;
	//ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	ObjectPtr<RenderTexture2D> mainRenderTexture;
	ObjectPtr<DescriptorHeap> bindlessTextureHeap;
	ObjectPtr<DescriptorHeap> uavTextureHeap;
	std::unordered_map<std::string, std::unique_ptr<FMaterial>> mMaterials;
	std::vector<ObjectPtr<Texture>> mTextures;
	ObjectPtr<Mesh> boxMesh;
	ObjectPtr<Mesh> sphereMesh;
//	Shader* opaqueShader;
	ObjectPtr<Material> opaqueMaterial;
	ObjectPtr<MeshRenderer> mainRenderer;
	ObjectPtr<MeshRenderer> mainRenderer1;
	ComPtr<ID3D12CommandSignature> mCommandSignature;
	ObjectPtr<Skybox> skybox;
	PassConstants mMainPassCB;
	ObjectPtr<Camera> mainCamera;
	ObjectPtr <UploadBuffer> materialPropertyBuffer;
	ObjectPtr <UploadBuffer> indirectDrawBuffer;
	float mTheta = 1.3f*XM_PI;
	float mPhi = 0.4f*XM_PI;
	float mRadius = 2.5f;
	POINT mLastMousePos;
	ThreadCommand* mainThreadCommand;
	ThreadCommand* separateThreadCommand;
	tf::Executor taskFlowExecutor;
	tf::Taskflow taskFlow;
	std::mutex mtx;

	StoragePSOContainer mainRTPsoContainer;
	StoragePSOContainer mirrorRTPsoContainter;
	StorageComputeShader testComputeShader;
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
}

CrateApp::~CrateApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool CrateApp::Initialize()
{

	if (!D3DApp::Initialize())
		return false;
	ShaderID::Init();
	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
	ShaderCompiler::Init(md3dDevice.Get());
	LoadTextures();
	BuildDescriptorHeaps();
	GeometryGenerator geoGen;
	BuildShapeGeometry(geoGen.CreateBox(1, 1, 1, 1), boxMesh);
	BuildShapeGeometry(geoGen.CreateSphere(1, 32, 33), sphereMesh);
	BuildMaterials();
	BuildFrameResources();
	BuildPSOs();
	mainCamera =new Camera(md3dDevice.Get());
	std::vector<ObjectPtr<Material>> mats(1);
	mats[0] = opaqueMaterial;
	XMFLOAT3 pos = { 0,-0.5,0 };
	XMVECTOR quat = { 0,0,0,1 };
	XMFLOAT3 localScale = { 0.5,0.5,0.5 };
	mainRenderer = new MeshRenderer(
		md3dDevice.Get(),
		pos,
		quat,
		localScale,
		boxMesh,
		mats
		);
	pos.y = 0.5;
	mainRenderer1 = new MeshRenderer(
		md3dDevice.Get(),
		pos,
		quat,
		localScale,
		sphereMesh,
		mats
	);
	//BuildPSOs();
	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void CrateApp::OnResize()
{
	D3DApp::OnResize();
}

void CrateApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	FrameResource::mCurrFrameResource = FrameResource::mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	FrameResource::mCurrFrameResource->UpdateBeforeFrame(mFence.Get());
	AnimateMaterials(gt);
	UpdateMaterialCBs(gt);
	mainCamera->SetLens(0.333333 * MathHelper::Pi, AspectRatio(), 1.5, 100);
	mainRenderer->UpdateObjectBuffer(FrameResource::mCurrFrameResource);
	mainRenderer1->UpdateObjectBuffer(FrameResource::mCurrFrameResource);
	mainCamera->UploadCameraBuffer(FrameResource::mCurrFrameResource, mMainPassCB);
	IndirectDrawCommand commands[2];
	mainRenderer->GetIndirectArgument(
		0,
		0,
		md3dDevice.Get(),
		FrameResource::mCurrFrameResource,
		commands
	);
	mainRenderer1->GetIndirectArgument(
		0,
		0,
		md3dDevice.Get(),
		FrameResource::mCurrFrameResource,
		commands + 1
	);
	indirectDrawBuffer->CopyDatas(0, 2, commands);
}
class DrawRenderTextureCommand
{
public:
	CrateApp* ths;
	ConstBufferElement* cameraBuffer;
	PSOContainer* separateContainer;
	void operator()()
	{
		ID3D12GraphicsCommandList* mSeparateList = ths->separateThreadCommand->GetCmdList();
		UploadBuffer::UploadData(mSeparateList);
		ths->mainRenderTexture->ClearRenderTarget(mSeparateList, true, true);
		ths->mainRenderTexture->SetViewport(mSeparateList);
		mSeparateList->OMSetRenderTargets(1, &ths->mainRenderTexture->GetColorHeap()->hCPU(0), true, &ths->mainRenderTexture->GetDepthHeap()->hCPU(0));
		ths->skybox->Draw(
			0,
			mSeparateList,
			ths->md3dDevice.Get(),
			cameraBuffer,
			FrameResource::mCurrFrameResource,
			separateContainer
		);
		/*ths->mainRenderer->Draw(
			1,
			0,
			mSeparateList,
			ths->md3dDevice.Get(),
			cameraBuffer,
			FrameResource::mCurrFrameResource,
			separateContainer
		);
		ths->mainRenderer1->Draw(
			1,
			0,
			mSeparateList,
			ths->md3dDevice.Get(),
			cameraBuffer,
			FrameResource::mCurrFrameResource,
			separateContainer
		);*/
		PSODescriptor desc;
		desc.meshLayoutIndex = ths->boxMesh->GetLayoutIndex();
		desc.shaderPass = 1;
		Material* mat = ths->opaqueMaterial.operator->();
		desc.shaderPtr = mat->GetShader();
		ID3D12PipelineState* pso = separateContainer->GetState(desc, ths->md3dDevice.Get());
		mSeparateList->SetPipelineState(pso);
		mat->BindShaderResource(mSeparateList);
		mSeparateList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ConstBufferElement* camBuffer = &FrameResource::mCurrFrameResource->cameraCBs[ths->mainCamera->GetInstanceID()];
		mat->GetShader()->SetResource(mSeparateList, ShaderID::GetPerCameraBufferID(), camBuffer->buffer.operator->(), camBuffer->element);
		mSeparateList->ExecuteIndirect(ths->mCommandSignature.Get(), 2, ths->indirectDrawBuffer->Resource(), 0, nullptr, 0);

		ComputeShader* cs = (ComputeShader*)&ths->testComputeShader;
		ths->mainRenderTexture->SetUav(true, mSeparateList);
		cs->BindRootSignature(mSeparateList, ths->uavTextureHeap.operator->());
		cs->SetResource(mSeparateList, ShaderID::PropertyToID("_MainTex"), ths->uavTextureHeap.operator->(), 0);
		const UINT ksize = 512 / 8;
		cs->Dispatch(mSeparateList, 0, ksize, ksize, 1);
	}
};
void CrateApp::Draw(const GameTimer& gt)
{
	mainThreadCommand = FrameResource::mCurrFrameResource->GetNewThreadCommand(md3dDevice.Get());
	separateThreadCommand = FrameResource::mCurrFrameResource->GetNewThreadCommand(md3dDevice.Get());
	mainThreadCommand->ResetCommand();
	separateThreadCommand->ResetCommand();
	ID3D12GraphicsCommandList* mCommandList = mainThreadCommand->GetCmdList();
	taskFlow.clear();
	ConstBufferElement* camBuffer = &FrameResource::mCurrFrameResource->cameraCBs[mainCamera->GetInstanceID()];
	PSOContainer* separatePSO = (PSOContainer*)&mirrorRTPsoContainter;
	DrawRenderTextureCommand func = { this, camBuffer, separatePSO };
	tf::Task firstTask = taskFlow.emplace(func);
	std::future<void> runner = taskFlowExecutor.run(taskFlow);
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
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
	/*mainRenderer->Draw(
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
	);*/
	PSODescriptor desc;
	desc.meshLayoutIndex = boxMesh->GetLayoutIndex();
	desc.shaderPass = 0;
	Material* mat = opaqueMaterial.operator->();
	desc.shaderPtr = mat->GetShader();
	ID3D12PipelineState* pso = mainPSO->GetState(desc, md3dDevice.Get());
	mCommandList->SetPipelineState(pso);
	mat->BindShaderResource(mCommandList);
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mat->GetShader()->SetResource(mCommandList, ShaderID::GetPerCameraBufferID(), camBuffer->buffer.operator->(), camBuffer->element);
	mCommandList->ExecuteIndirect(mCommandSignature.Get(), 2, indirectDrawBuffer->Resource(), 0, nullptr, 0);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	ID3D12CommandList* value[2];
	UINT count = 0;
	runner.wait();
	separateThreadCommand->CollectCommand(value, &count);
	mainThreadCommand->CollectCommand(value, &count);
	mCommandQueue->ExecuteCommandLists(_countof(value), value);
	FrameResource::mCurrFrameResource->ReleaseThreadCommand(mainThreadCommand);
	FrameResource::mCurrFrameResource->ReleaseThreadCommand(separateThreadCommand);
	
	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
	FrameResource::mCurrFrameResource->UpdateAfterFrame(mCurrentFence, mCommandQueue.Get(), mFence.Get());

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

void CrateApp::AnimateMaterials(const GameTimer& gt)
{

}
void CrateApp::UpdateMaterialCBs(const GameTimer& gt)
{
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
}

void CrateApp::LoadTextures()
{
	mTextures.reserve(12);
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
	mTextures.push_back(brickTex);
	mainRenderTexture = new RenderTexture2D(md3dDevice.Get(), 1024, 1024, DXGI_FORMAT_R16G16B16A16_FLOAT, 2, true);
}

void CrateApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
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
	mainRenderTexture->GetUAVViewDesc(uavDesc, 0);
	md3dDevice->CreateUnorderedAccessView(mainRenderTexture->GetColorResource(), nullptr, &uavDesc, uavTextureHeap->hCPU(0));
	mainRenderTexture->GetUAVViewDesc(uavDesc, 1);
	md3dDevice->CreateUnorderedAccessView(mainRenderTexture->GetColorResource(), nullptr, &uavDesc, uavTextureHeap->hCPU(1));
}

void CrateApp::BuildShapeGeometry(GeometryGenerator::MeshData& box, ObjectPtr<Mesh>& bMesh)
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
		md3dDevice.Get(),
		mCommandList.Get(),
		subMeshs
		);

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

}

void CrateApp::BuildPSOs()
{
	DXGI_FORMAT colorFormat = mainRenderTexture->GetColorFormat();
	new (&mirrorRTPsoContainter)PSOContainer(mainRenderTexture->GetDepthFormat(), 1, &colorFormat);
	new(&mainRTPsoContainer)PSOContainer(mDepthStencilFormat, 1, &mBackBufferFormat);
	std::vector<std::string> kernelNames(1);
	kernelNames[0] = "CSMain";
	std::vector<ComputeShaderVariable> vars(1);
	vars[0].space = 0;
	vars[0].registerPos = 0;
	vars[0].type = ComputeShaderVariable::UAVDescriptorHeap;
	vars[0].tableSize = 2;
	vars[0].name = "_MainTex";
	new(&testComputeShader)ComputeShader(L"Shaders\\ComputeShaderTest.hlsl", kernelNames, vars, md3dDevice.Get());
	D3D12_COMMAND_SIGNATURE_DESC desc = {};
	D3D12_INDIRECT_ARGUMENT_DESC indDesc[4];
	ZeroMemory(indDesc, 4 * sizeof(D3D12_INDIRECT_ARGUMENT_DESC));
	indDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	indDesc[0].ConstantBufferView.RootParameterIndex = opaqueMaterial->GetShader()->GetPropertyRootSigPos(ShaderID::GetPerObjectBufferID());
	indDesc[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
	indDesc[1].VertexBuffer.Slot = 0;
	indDesc[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
	indDesc[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	desc.ByteStride = sizeof(IndirectDrawCommand);
	desc.NodeMask = 0;
	desc.NumArgumentDescs = 4;
	desc.pArgumentDescs = indDesc;
	md3dDevice->CreateCommandSignature(&desc, opaqueMaterial->GetShader()->GetSignature(), IID_PPV_ARGS(&mCommandSignature));
	indirectDrawBuffer = new UploadBuffer();
	indirectDrawBuffer->Create(md3dDevice.Get(), 2, false, sizeof(IndirectDrawCommand), false);
}