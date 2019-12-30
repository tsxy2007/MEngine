#include "Graphics.h"
#include "../RenderComponent/Shader.h"
#include "MeshLayout.h"
#include "../RenderComponent/Mesh.h"
#include "ShaderCompiler.h"
#include "../Singleton/PSOContainer.h"
std::unique_ptr<Mesh> fullScreenMesh(nullptr);
void Graphics::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{

	std::array<DirectX::XMFLOAT3, 3> vertex;
	std::array<DirectX::XMFLOAT2, 3> uv;
	vertex[0] = { -3, -1, 1 };
	vertex[1] = { 1, -1, 1 };
	vertex[2] = { 1, 3, 1 };
	uv[0] = { -1, 1 };
	uv[1] = { 1, 1 };
	uv[2] = { 1, -1 };
	std::array<INT16, 3> indices = { 0, 1, 2 };
	std::vector<SubMesh>* subMeshes = new std::vector<SubMesh>(1);
	SubMesh& sm = (*subMeshes)[0];
	sm.boundingCenter = { 0,0,0 };
	sm.boundingExtent = { 1,1,1 };
	sm.indexCount = 3;
	sm.indexFormat = DXGI_FORMAT_R16_UINT;
	sm.indexArrayPtr = indices.data();
	fullScreenMesh = std::make_unique<Mesh>(
		3,
		vertex.data(),
		nullptr,
		nullptr,
		nullptr,
		uv.data(),
		nullptr,
		nullptr,
		nullptr,
		device,
		commandList,
		subMeshes
		);

}


void Graphics::Blit(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	D3D12_CPU_DESCRIPTOR_HANDLE* renderTarget,
	UINT renderTargetCount,
	D3D12_CPU_DESCRIPTOR_HANDLE* depthTarget,
	PSOContainer* container,
	UINT width, UINT height,
	Shader* shader, UINT pass)
{
	D3D12_VIEWPORT mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	D3D12_RECT mScissorRect = { 0, 0, (int)width, (int)height };
	PSODescriptor psoDesc;
	psoDesc.meshLayoutIndex = fullScreenMesh->GetLayoutIndex();
	psoDesc.shaderPass = pass;
	psoDesc.shaderPtr = shader;
	commandList->OMSetRenderTargets(renderTargetCount, renderTarget, true, depthTarget);
	commandList->RSSetViewports(1, &mViewport);
	commandList->RSSetScissorRects(1, &mScissorRect);
	commandList->SetPipelineState(container->GetState(psoDesc, device));
	commandList->IASetVertexBuffers(0, 1, &fullScreenMesh->VertexBufferView());
	commandList->IASetIndexBuffer(&fullScreenMesh->IndexBufferView(0));
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	SubMesh& subMesh = fullScreenMesh->GetSubmesh(0);
	commandList->DrawIndexedInstanced(subMesh.indexCount, 1, 0, 0, 0);
}
/*	UINT64 n64RequiredSize = 0u;
UINT   nNumSubresources = 1u;
D3D12_PLACED_SUBRESOURCE_FOOTPRINT stTxtLayouts = {};
UINT64 n64TextureRowSizes = 0u;
UINT   nTextureRowNum = 0u;
D3D12_RESOURCE_DESC destDesc = backBuffer->GetDesc();
device->GetCopyableFootprints(&destDesc
	, 0
	, nNumSubresources
	, 0
	, &stTxtLayouts
	, &nTextureRowNum
	, &n64TextureRowSizes
	, &n64RequiredSize);
CD3DX12_TEXTURE_COPY_LOCATION Dst(backBuffer, 0);
CD3DX12_TEXTURE_COPY_LOCATION Src(rt->GetColorResource(), stTxtLayouts);
commandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);*/
//commandList->ClearRenderTargetView(backBufferHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);

//sky->Draw()
//postProcessingShader->BindRootSignature(commandList);
//Graphics::Blit(commandList, device, &backBufferHandle, 1, nullptr, gbufferContainer, rt->GetWidth(), rt->GetHeight(), postProcessingShader, 0);