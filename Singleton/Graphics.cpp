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
	vertex[1] = { 1, 3, 1 };
	vertex[2] = { 1, -1, 1 };
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
	D3D12_CPU_DESCRIPTOR_HANDLE renderTarget,
	PSOContainer* container,
	Shader* shader, UINT pass)
{
	PSODescriptor psoDesc;
	psoDesc.meshLayoutIndex = fullScreenMesh->GetLayoutIndex();
	psoDesc.shaderPass = pass;
	psoDesc.shaderPtr = shader;
	commandList->OMSetRenderTargets(1, &renderTarget, true, nullptr);
	float colors[4] = { 0,1,1,1 };
	commandList->ClearRenderTargetView(renderTarget, colors, 0, nullptr);
	commandList->SetPipelineState(container->GetState(psoDesc, device));
	commandList->IASetVertexBuffers(0, 1, &fullScreenMesh->VertexBufferView());
	commandList->IASetIndexBuffer(&fullScreenMesh->IndexBufferView(0));
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	SubMesh& subMesh = fullScreenMesh->GetSubmesh(0);
	commandList->DrawIndexedInstanced(subMesh.indexCount, 1, 0, 0, 0);
}