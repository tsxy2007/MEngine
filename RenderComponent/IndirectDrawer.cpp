#include "IndirectDrawer.h"
#include "../Singleton/ShaderID.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../Singleton/ShaderCompiler.h"
#include "../RenderComponent/ComputeShader.h"
using namespace Microsoft::WRL;

void IndirectDrawer::Draw(
	int targetPass,
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	ConstBufferElement* cameraBuffer,
	FrameResource* currentResource,
	PSOContainer* container,
	DescriptorHeap* srvHeap,
	HeapSet* heapSets,
	UINT heapSetCount
)
{
	if (allMeshCommands.size() <= 0) return;
	ComputeShader* cullCS = ShaderCompiler::GetComputeShader("Cull");
	cullCS->BindRootSignature(commandList, nullptr);
	cullCS->SetResource(commandList, ShaderID::PropertyToID("CBuffer"), &csConstBuffer, 0);
	cullCS->SetResource(commandList, ShaderID::PropertyToID("_InputBuffer"), &indirectDataBuffer, 0);
	cullCS->SetStructuredBufferByAddress(commandList, ShaderID::PropertyToID("_OutputBuffer"), indirectDrawBuffer->GetGPUVirtualAddress());
	cullCS->SetStructuredBufferByAddress(commandList, ShaderID::PropertyToID("_CountBuffer"), indirectDrawBuffer->GetGPUVirtualAddress() + sizeof(MultiDrawCommand) * allMeshCommands.size());
	cullCS->Dispatch(commandList, 1, 1, 1, 1);
	UINT disp = (UINT)ceil(allMeshCommands.size() / 64.0);
	cullCS->Dispatch(commandList, 0, disp, 1, 1);
	PSODescriptor desc;
	desc.meshLayoutIndex = allMeshCommands[0].mesh->GetLayoutIndex();
	desc.shaderPass = targetPass;
	desc.shaderPtr = mShader;
	ID3D12PipelineState* pso = container->GetState(desc, device);
	commandList->SetPipelineState(pso);
	mShader->BindRootSignature(commandList);
	commandList->SetDescriptorHeaps(1, srvHeap->Get().GetAddressOf());
	for (UINT i = 0; i < heapSetCount; ++i)
	{
		mShader->SetResource(commandList, heapSets[i].shaderID, srvHeap, heapSets[i].heapOffset);
	}
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mShader->SetResource(commandList, ShaderID::GetPerCameraBufferID(), cameraBuffer->buffer, cameraBuffer->element);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indirectDrawBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));
	commandList->ExecuteIndirect(cmdSig.GetSignature(), allMeshCommands.size(), indirectDrawBuffer.Get(), 0, indirectDrawBuffer.Get(), sizeof(MultiDrawCommand) * allMeshCommands.size());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indirectDrawBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

IndirectDrawer::IndirectDrawer(
	Shader* targetShader,
	MeshCommand* commands,
	UINT commandCount,
	size_t objectBufferStride,
	size_t materialBufferStride,
	UINT materialCount,
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList
) : MObject(),
mShader(targetShader),
allMeshCommands(commandCount), 
cmdSig(targetShader, device)
{
	memcpy(allMeshCommands.data(), commands, sizeof(MeshCommand) * commandCount);
	csConstBuffer.Create(device, 1, true, sizeof(CullShaderData));
	CullShaderData data = { commandCount };
	csConstBuffer.CopyData(0, &data);
	indirectDataBuffer.Create(device, commandCount, false, sizeof(MultiDrawCommand));
	materialBuffers.Create(device, materialCount, true, materialBufferStride);
	objectBuffers.Create(device, commandCount, true, objectBufferStride);
	std::vector<MultiDrawCommand> tempCommand(commandCount);
	for (UINT i = 0; i < commandCount; ++i)
	{
		/*MultiDrawCommand& t = tempCommand[i];
		MeshCommand& c = commands[i];
		SubMesh& subm = c.mesh->GetSubmesh(c.subMeshIndex);
		t.vertexBuffer = c.mesh->VertexBufferView();
		t.indexBuffer = c.mesh->IndexBufferView(c.subMeshIndex);
		t.materialBufferAddress = materialBuffers.Resource()->GetGPUVirtualAddress() + c.materialIndex * materialBuffers.GetAlignedStride();
		t.objectCBufferAddress = objectBuffers.Resource()->GetGPUVirtualAddress() + i * objectBuffers.GetAlignedStride();
		t.drawArgs.BaseVertexLocation = 0;
		t.drawArgs.IndexCountPerInstance = subm.indexCount;
		t.drawArgs.InstanceCount = 1;
		t.drawArgs.StartIndexLocation = 0;
		t.drawArgs.StartInstanceLocation = 0;*/
	}
	indirectDataBuffer.CopyDatas(0, commandCount, tempCommand.data());
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(MultiDrawCommand) * commandCount + sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&indirectDrawBuffer)));
}

IndirectDrawer::~IndirectDrawer()
{
}

void IndirectDrawer::UploadObjectBuffer(
	const void* dataPtr,
	UINT pos
)
{
	objectBuffers.CopyData(pos, dataPtr);
}

void IndirectDrawer::UploadMaterialBuffer(
	const void* dataPtr,
	UINT pos
)
{
	materialBuffers.CopyData(pos, dataPtr);
}