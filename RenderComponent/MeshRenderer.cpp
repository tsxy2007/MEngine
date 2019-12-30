#include "MeshRenderer.h"
#include "../Singleton/PSOContainer.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/FrameResource.h"
#include "../LogicComponent/Transform.h"
using namespace DirectX;
CBufferPool MeshRenderer::objectPool(sizeof(ObjectConstants), 256);
MeshRenderer::MeshRenderer(
	Transform* trans,
	ID3D12Device* device,
	ObjectPtr<Mesh>& initMesh,
	std::vector<ObjectPtr<Material>>& allMaterials
) : Component(trans), mMaterials(allMaterials.size()), mesh(initMesh)
{
	for (int i = 0; i < allMaterials.size(); ++i)
	{
		mMaterials[i] = allMaterials[i];
	}
	for (int i = 0; i < FrameResource::mFrameResources.size(); ++i)
	{
		FrameResource* ptr = FrameResource::mFrameResources[i].get();
		ptr->objectCBs[GetInstanceID()] = objectPool.GetBuffer(device);
	}
}

MeshRenderer::~MeshRenderer()
{
	for (int i = 0; i < FrameResource::mFrameResources.size(); ++i)
	{
		FrameResource* ptr = FrameResource::mFrameResources[i].get();
		auto&& ite = ptr->objectCBs.find(GetInstanceID());
		if (ite != ptr->objectCBs.end())
		{
			objectPool.Release(ite->second);
			ptr->objectCBs.erase(GetInstanceID());
		}
	}
}

void MeshRenderer::Draw(
	int targetPass,
	int targetSubMesh,
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	ConstBufferElement* cameraBuffer,
	FrameResource* currentResource,
	PSOContainer* container
)
{
	PSODescriptor desc;
	desc.meshLayoutIndex = mesh->GetLayoutIndex();
	desc.shaderPass = targetPass;
	Material* mat = mMaterials[targetSubMesh].operator->();
	desc.shaderPtr = mat->GetShader();
	ID3D12PipelineState* pso = container->GetState(desc, device);
	commandList->SetPipelineState(pso);
	mat->BindShaderResource(commandList);
	ConstBufferElement& objBuffer = currentResource->objectCBs[GetInstanceID()];
	mat->GetShader()->SetResource(commandList, ShaderID::GetPerCameraBufferID(), cameraBuffer->buffer.operator->(), cameraBuffer->element);
	mat->GetShader()->SetResource(commandList, ShaderID::GetPerObjectBufferID(), objBuffer.buffer.operator->(), objBuffer.element);
	commandList->IASetVertexBuffers(0, 1, &mesh->VertexBufferView());
	commandList->IASetIndexBuffer(&mesh->IndexBufferView(targetSubMesh));
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	SubMesh& subMesh = mesh->GetSubmesh(targetSubMesh);
	commandList->DrawIndexedInstanced(subMesh.indexCount, 1, 0, 0, 0);
}

void MeshRenderer::GetIndirectArgument(
	int targetPass,
	int targetSubMesh,
	ID3D12Device* device,
	FrameResource* currentResource,
	IndirectDrawCommand* command
)
{
	SubMesh& subMesh = mesh->GetSubmesh(targetSubMesh);
	ConstBufferElement& objBuffer = currentResource->objectCBs[GetInstanceID()];
	command->indexBuffer = mesh->IndexBufferView(targetSubMesh);
	command->objectCBufferAddress = objBuffer.buffer->Resource()->GetGPUVirtualAddress() + objBuffer.element * objBuffer.buffer->GetAlignedStride();
	command->vertexBuffer = mesh->VertexBufferView();
	command->drawArgs.BaseVertexLocation = 0;
	command->drawArgs.IndexCountPerInstance = subMesh.indexCount;
	command->drawArgs.InstanceCount = 1;
	command->drawArgs.StartIndexLocation = 0;
	command->drawArgs.StartInstanceLocation = 0;
}
void MeshRenderer::UpdateObjectBuffer(FrameResource* resource)
{
	ObjectConstants buffer;
	//XMStoreFloat4x4(&buffer.objectToWorld, transform.GetLocalToWorldMatrix());
	ConstBufferElement ele = resource->objectCBs[GetInstanceID()];
	ele.buffer->CopyData(ele.element, &buffer);
}