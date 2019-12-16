#include "IndirectDrawer.h"
#include "../Singleton/ShaderID.h"
using namespace Microsoft::WRL;
IndirectDrawer::IndirectDrawer(
	Shader* targetShader,
	MeshCommand* commands,
	size_t objectBufferStride,
	UINT commandCount,
	size_t materialBufferStride,
	UINT materialCount,
	ID3D12Device* device
) :
	mShader(targetShader),
	allMeshCommands(commandCount)
{
	memcpy(allMeshCommands.data(), commands, sizeof(MeshCommand) * commandCount);
	D3D12_COMMAND_SIGNATURE_DESC desc = {};
	D3D12_INDIRECT_ARGUMENT_DESC indDesc[5];
	ZeroMemory(indDesc, 5 * sizeof(D3D12_INDIRECT_ARGUMENT_DESC));
	indDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	indDesc[0].ConstantBufferView.RootParameterIndex = targetShader->GetPropertyRootSigPos(ShaderID::GetPerObjectBufferID());
	indDesc[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	indDesc[1].ConstantBufferView.RootParameterIndex = targetShader->GetPropertyRootSigPos(ShaderID::GetPerMaterialBufferID());
	indDesc[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
	indDesc[2].VertexBuffer.Slot = 0;
	indDesc[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
	indDesc[4].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	desc.ByteStride = sizeof(MultiDrawCommand);
	desc.NodeMask = 0;
	desc.NumArgumentDescs = 4;
	desc.pArgumentDescs = indDesc;
	device->CreateCommandSignature(&desc, targetShader->GetSignature(), IID_PPV_ARGS(&mCommandSignature));
	indirectDataBuffer.Create(device, commandCount, false, sizeof(MultiDrawCommand), false);
	materialBuffers.Create(device, materialCount, true, materialBufferStride, false);
	objectBuffers.Create(device, commandCount, true, objectBufferStride, false);
	MultiDrawCommand* tempCommand = new MultiDrawCommand[commandCount];//Use Heap prevent stack overflow
	for (UINT i = 0; i < commandCount; ++i)
	{
		MultiDrawCommand& t = tempCommand[i];
		MeshCommand& c = commands[i];
		SubMesh& subm = c.mesh->GetSubmesh(i);
		t.vertexBuffer = c.mesh->VertexBufferView();
		t.indexBuffer = c.mesh->IndexBufferView(c.subMeshIndex);
		t.materialBufferAddress = materialBuffers.Resource()->GetGPUVirtualAddress() + c.materialIndex * materialBuffers.GetAlignedStride();
		t.objectCBufferAddress = objectBuffers.Resource()->GetGPUVirtualAddress() + i * objectBuffers.GetAlignedStride();
		t.drawArgs.BaseVertexLocation = 0;
		t.drawArgs.IndexCountPerInstance = subm.indexCount;
		t.drawArgs.InstanceCount = 1;
		t.drawArgs.StartIndexLocation = 0;
		t.drawArgs.StartInstanceLocation = 0;
	}
	indirectDataBuffer.CopyDatas(0, commandCount, tempCommand);
	delete[] tempCommand;
}

void IndirectDrawer::Draw(
	int targetPass,
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	ConstBufferElement* cameraBuffer,
	FrameResource* currentResource,
	PSOContainer* container
)
{
	if (allMeshCommands.size() <= 0) return;

	PSODescriptor desc;
	desc.meshLayoutIndex = allMeshCommands[0].mesh->GetLayoutIndex();
	desc.shaderPass = targetPass;
	desc.shaderPtr = mShader;
	ID3D12PipelineState* pso = container->GetState(desc, device);
	commandList->SetPipelineState(pso);
	mShader->BindRootSignature(commandList);
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mShader->SetResource(commandList, ShaderID::GetPerCameraBufferID(), cameraBuffer->buffer.operator->(), cameraBuffer->element);
	commandList->ExecuteIndirect(mCommandSignature.Get(), allMeshCommands.size(), indirectDataBuffer.Resource(), 0, nullptr, 0);
}