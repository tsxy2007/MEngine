#include "Graphics.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/Shader.h"
#include "MeshLayout.h"
#include "../RenderComponent/Mesh.h"
#include "ShaderCompiler.h"
#include "../Singleton/PSOContainer.h"
#include "../RenderComponent/RenderTexture.h"
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
	fullScreenMesh = std::unique_ptr<Mesh>(new Mesh(
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
		DXGI_FORMAT_R16_UINT,
		3,
		indices.data()
		));

}
template <bool sourceIsDepth, bool destIsDepth>
inline void Copy(
	D3D12_TEXTURE_COPY_LOCATION& sourceLocation,
	D3D12_TEXTURE_COPY_LOCATION& destLocation,
	ID3D12GraphicsCommandList* commandList)
{
	const D3D12_RESOURCE_STATES sourceFormat = sourceIsDepth ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_RENDER_TARGET;
	const D3D12_RESOURCE_STATES destFormat = destIsDepth ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_RENDER_TARGET;
	Graphics::ResourceStateTransform<sourceFormat, D3D12_RESOURCE_STATE_COPY_SOURCE>(commandList, sourceLocation.pResource);
	Graphics::ResourceStateTransform<destFormat, D3D12_RESOURCE_STATE_COPY_DEST>(commandList, destLocation.pResource);
	commandList->CopyTextureRegion(
		&destLocation,
		0, 0, 0,
		&sourceLocation,
		nullptr
	);
	Graphics::ResourceStateTransform<D3D12_RESOURCE_STATE_COPY_SOURCE, sourceFormat>(commandList, sourceLocation.pResource);
	Graphics::ResourceStateTransform<D3D12_RESOURCE_STATE_COPY_DEST, destFormat>(commandList, destLocation.pResource);
}

void Graphics::CopyTexture(
	ID3D12GraphicsCommandList* commandList,
	RenderTexture* source, CopyTarget sourceTarget,
	RenderTexture* dest, CopyTarget destTarget)
{
	D3D12_TEXTURE_COPY_LOCATION sourceLocation;
	sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	sourceLocation.SubresourceIndex = 0;
	D3D12_TEXTURE_COPY_LOCATION destLocation;
	destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destLocation.SubresourceIndex = 0;
	if (sourceTarget == CopyTarget_ColorBuffer)
	{
		if (destTarget == CopyTarget_ColorBuffer)    //Source Color, Dest Color
		{
			sourceLocation.pResource = source->GetColorResource();
			destLocation.pResource = dest->GetColorResource();
			Copy<false, false>(sourceLocation, destLocation, commandList);
		}
		else    //Source Color, Dest Depth
		{
			sourceLocation.pResource = source->GetColorResource();
			destLocation.pResource = dest->GetDepthResource();
			Copy<false, true>(sourceLocation, destLocation, commandList);
		}
	}
	else
	{
		if (destTarget == CopyTarget_ColorBuffer)    //Source Depth, Dest Color
		{
			sourceLocation.pResource = source->GetDepthResource();
			destLocation.pResource = dest->GetColorResource();
			Copy<true, false>(sourceLocation, destLocation, commandList);
		}
		else    //Source Depth, Dest Depth
		{
			sourceLocation.pResource = source->GetDepthResource();
			destLocation.pResource = dest->GetDepthResource();
			Copy<true, true>(sourceLocation, destLocation, commandList);
		}
	}
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
	commandList->IASetIndexBuffer(&fullScreenMesh->IndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(fullScreenMesh->GetIndexCount(), 1, 0, 0, 0);
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