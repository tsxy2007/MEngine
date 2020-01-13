#include "MeshRenderer.h"
#include "../Singleton/PSOContainer.h"
#include "../Singleton/ShaderID.h"
#include "../LogicComponent/Transform.h"
using namespace DirectX;

std::vector<std::pair<MeshRenderer*, MeshRenderer::MeshRendererObjectData>> MeshRenderer::allRendererData;
MeshRenderer::MeshRenderer(
	Transform* trans,
	ID3D12Device* device,
	ObjectPtr<Mesh>& initMesh,
	ObjectPtr<Material>& allMaterial
) : Component(trans), mMaterial(allMaterial), mesh(initMesh)
{
	MeshRendererObjectData data;
	data.boundingCenter = mesh->boundingCenter;
	data.boundingExtent = mesh->boundingExtent;
	data.localToWorld = trans->GetLocalToWorldMatrix();
	listIndex = allRendererData.size();
	allRendererData.emplace_back<MeshRenderer*, MeshRenderer::MeshRendererObjectData>(this, std::move(data));
}

MeshRenderer::~MeshRenderer()
{
	auto&& ite = allRendererData.end() - 1;
	allRendererData[listIndex] = *ite;
	ite->first->listIndex = listIndex;
	allRendererData.erase(ite);
}

void MeshRenderer::Draw(
	int targetPass,
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	ConstBufferElement* cameraBuffer,
	UploadBuffer* objectBuffer,
	UINT objectBufferOffset,
	PSOContainer* container
)
{
	PSODescriptor desc;
	desc.meshLayoutIndex = mesh->GetLayoutIndex();
	desc.shaderPass = targetPass;
	Material* mat = mMaterial.operator->();
	desc.shaderPtr = mat->GetShader();
	ID3D12PipelineState* pso = container->GetState(desc, device);
	commandList->SetPipelineState(pso);
	mat->BindShaderResource(commandList);
	mat->GetShader()->SetResource(commandList, ShaderID::GetPerCameraBufferID(), cameraBuffer->buffer, cameraBuffer->element);
	mat->GetShader()->SetResource(commandList, ShaderID::GetPerObjectBufferID(), objectBuffer, objectBufferOffset);
	commandList->IASetVertexBuffers(0, 1, &mesh->VertexBufferView());
	commandList->IASetIndexBuffer(&mesh->IndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
}