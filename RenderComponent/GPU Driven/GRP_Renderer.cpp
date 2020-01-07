#include "GRP_Renderer.h"
#include "../DescriptorHeap.h"
#include "../Mesh.h"
#include "../../LogicComponent/Transform.h"
#include "../../Singleton/ShaderCompiler.h"
#include "../ComputeShader.h"
#include "../../Singleton/FrameResource.h"
#include "../StructuredBuffer.h"
#include "../../Common/MetaLib.h"
#include "../../Singleton/ShaderID.h"
#include "../../Singleton/PSOContainer.h"
#include <mutex>
struct ObjectData
{
	DirectX::XMFLOAT4X4 localToWorld;
	DirectX::XMFLOAT3 boundingCenter;
	DirectX::XMFLOAT3 boundingExtent;
};
struct CullData
{
	UINT _Count;
	DirectX::XMFLOAT4 planes[6];
	DirectX::XMFLOAT3 _FrustumMinPoint;
	DirectX::XMFLOAT3 _FrustumMaxPoint;
};
struct Command
{
	enum CommandType
	{
		CommandType_Add,
		CommandType_Remove,
		CommandType_TransformPos,
		CommandType_Renderer
	};
	CommandType type;
	UINT index;
	MultiDrawCommand cmd;
	ObjectData objData;
	Command() {}
	Command(const Command& cmd)
	{
		memcpy(this, &cmd, sizeof(Command));
	}
};
class GpuDrivenRenderer : public IPipelineResource
{
public:
	std::unique_ptr<UploadBuffer> objectPosBuffer;
	std::unique_ptr<UploadBuffer> cmdDrawBuffers;
	CBufferPool objectPool;
	std::mutex mtx;
	UINT capacity;
	UINT count;
	std::vector<Command> allCommands;
	std::vector<ConstBufferElement> allocatedObjects;
	GpuDrivenRenderer(
		ID3D12Device* device,
		UINT capacity
	) : capacity(capacity), count(0),
		objectPool(sizeof(ObjectConstants), capacity)
	{
		allocatedObjects.reserve(capacity);
		objectPosBuffer = std::unique_ptr<UploadBuffer>(new UploadBuffer());
		cmdDrawBuffers = std::unique_ptr<UploadBuffer>(new UploadBuffer());
		objectPosBuffer->Create(device, capacity, false, sizeof(ObjectData));
		cmdDrawBuffers->Create(device, capacity, false, sizeof(MultiDrawCommand));
		
	}

	void Resize(
		UINT targetCapacity,
		ID3D12Device* device)
	{
		if (targetCapacity <= capacity) return;
		UINT autoCapac = (UINT)(capacity * 1.5);
		targetCapacity = max(targetCapacity, autoCapac);
		UploadBuffer* newObjBuffer = new UploadBuffer();
		UploadBuffer* newCmdDrawBuffer = new UploadBuffer();
		newObjBuffer->Create(device, targetCapacity, false, sizeof(ObjectData));
		newCmdDrawBuffer->Create(device, targetCapacity, false, sizeof(MultiDrawCommand));
		newObjBuffer->CopyFrom(objectPosBuffer.get(), 0, 0, capacity);
		newCmdDrawBuffer->CopyFrom(cmdDrawBuffers.get(), 0, 0, capacity);

		objectPosBuffer = std::unique_ptr<UploadBuffer>(newObjBuffer);
		cmdDrawBuffers = std::unique_ptr<UploadBuffer>(newCmdDrawBuffer);
		capacity = targetCapacity;
	}

	void AddCommand(Command& cmd)
	{
		std::lock_guard<std::mutex> lck(mtx);
		allCommands.push_back(cmd);
	}

	void UpdateFrame(
		ID3D12Device* device
	)
	{
		for (UINT i = 0; ; ++i)
		{
			Command c;
			mtx.lock();
			if (i >= allCommands.size())
			{
				goto END_OF_LOOP;
			}
			c = allCommands[i];
			mtx.unlock();
			ConstBufferElement& cEle = allocatedObjects[c.index];
			auto&& ite = allocatedObjects.end() - 1;
			UINT last = count - 1;
			switch (c.type)
			{
			case Command::CommandType_Add:
				Resize(count + 1, device);
				ConstBufferElement obj = objectPool.Get(device);
				c.cmd.objectCBufferAddress = obj.buffer->Resource()->GetGPUVirtualAddress() +
					obj.element * obj.buffer->GetAlignedStride();
				allocatedObjects.push_back(obj);
				objectPosBuffer->CopyData(count, &c.objData);
				cmdDrawBuffers->CopyData(count, &c.cmd);
				count++;
				break;
			case Command::CommandType_Remove:
				if (c.index != last)
				{
					objectPosBuffer->CopyDataInside(last, c.index);
					cmdDrawBuffers->CopyDataInside(last, c.index);
				}
				objectPool.Return(cEle);
				cEle = *ite;
				allocatedObjects.erase(ite);
				count--;
				break;
			case Command::CommandType::CommandType_TransformPos:
				objectPosBuffer->CopyData(c.index, &c.objData);
				break;
			case Command::CommandType::CommandType_Renderer:
				c.cmd.objectCBufferAddress = cEle.buffer->Resource()->GetGPUVirtualAddress() +
					cEle.element * cEle.buffer->GetAlignedStride();
				cmdDrawBuffers->CopyData(c.index, &c.cmd);
				break;
			}
		}
	END_OF_LOOP:
		allCommands.clear();
		mtx.unlock();
	}

	~GpuDrivenRenderer()
	{
	}
};
GRP_Renderer::GRP_Renderer(
	size_t materialPropertyStride,
	UINT meshLayoutIndex,
	UINT initCapacity,
	UINT maxCapacity,
	Shader* shader,
	UINT texRequireInMat,
	ID3D12Device* device
) :
	MObject(),
	capacity(initCapacity),
	pool(materialPropertyStride, initCapacity),
	cbufferStride(materialPropertyStride),
	shader(shader),
	maxCapacity(maxCapacity),
	cmdSig(shader, device),
	textureHeap(new DescriptorHeap()),
	textureDescPool(maxCapacity * texRequireInMat),
	texRequireInMat(texRequireInMat),
	textureIndicesPool(maxCapacity),
	meshLayoutIndex(meshLayoutIndex)
{
	cullShader = ShaderCompiler::GetComputeShader("Cull");
	textureHeap->Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, maxCapacity * texRequireInMat, true);
	for (UINT i = 0, size = textureDescPool.size(); i < size; ++i)
		textureDescPool[i] = i;
	allocatedIndices = new UINT[maxCapacity * texRequireInMat];
	for (UINT i = 0; i < maxCapacity; ++i)
	{
		textureIndicesPool[i] = allocatedIndices + i * texRequireInMat;
	}
	StructuredBufferElement ele[2];
	ele[0].elementCount = capacity;
	ele[0].stride = sizeof(MultiDrawCommand);
	ele[1].elementCount = 1;
	ele[1].stride = sizeof(UINT);
	cullResultBuffer = std::unique_ptr<StructuredBuffer>(new StructuredBuffer(
		device,
		ele,
		2,
		true
	));


	_InputBuffer = ShaderID::PropertyToID("_InputBuffer");
	_InputDataBuffer = ShaderID::PropertyToID("_InputDataBuffer");
	_OutputBuffer = ShaderID::PropertyToID("_OutputBuffer");
	_CountBuffer = ShaderID::PropertyToID("_CountBuffer");
	CBuffer = ShaderID::PropertyToID("CBuffer");
}

GRP_Renderer::RenderElement& GRP_Renderer::AddRenderElement(
	ObjectPtr<Transform>& targetTrans,
	ObjectPtr<Mesh>& mesh,
	ID3D12Device* device
)
{
	if (elements.size() >= maxCapacity)
		throw "Out of Range!";
	if (mesh->GetLayoutIndex() != meshLayoutIndex)
		throw "Mesh Bad Layout!";
	auto&& ite = dicts.find(std::forward<Transform*>(targetTrans.operator->()));
	if (ite != dicts.end())
	{
		return elements[ite->second];
	}
	dicts.insert_or_assign(std::forward<Transform*>(targetTrans.operator->()), elements.size());
	auto&& indPtrIte = textureIndicesPool.end() - 1;
	RenderElement& ele = elements.emplace_back(
		std::move(pool.Get(device)),
		&targetTrans,
		*indPtrIte,
		mesh->boundingCenter,
		mesh->boundingExtent
	);
	textureIndicesPool.erase(indPtrIte);
	for (UINT i = 0; i < texRequireInMat; ++i)
	{
		auto&& ite = textureDescPool.end() - 1;
		ele.textures[i] = *ite;
		textureDescPool.erase(ite);
	}

	Command cmd;
	cmd.index = elements.size() - 1;
	cmd.type = Command::CommandType_Add;
	cmd.cmd.objectCBufferAddress = 0;
	cmd.cmd.materialBufferAddress = ele.propertyBuffer.buffer->Resource()->GetGPUVirtualAddress()
		+ ele.propertyBuffer.element * ele.propertyBuffer.buffer->GetAlignedStride();
	cmd.cmd.vertexBuffer = mesh->VertexBufferView();
	cmd.cmd.indexBuffer = mesh->IndexBufferView();
	cmd.cmd.drawArgs.BaseVertexLocation = 0;
	cmd.cmd.drawArgs.IndexCountPerInstance = mesh->GetIndexCount();
	cmd.cmd.drawArgs.InstanceCount = 1;
	cmd.cmd.drawArgs.StartIndexLocation = 0;
	cmd.cmd.drawArgs.StartInstanceLocation = 0;
	cmd.objData.boundingCenter = ele.boundingCenter;
	cmd.objData.boundingExtent = ele.boundingExtent;
	DirectX::XMMATRIX* ptr = (DirectX::XMMATRIX*) &cmd.objData.localToWorld;
	*ptr = targetTrans->GetLocalToWorldMatrix();
	for (UINT i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)this->container.GetResource(&FrameResource::mFrameResources[i]->resourceManager, this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(device, capacity);
		});
		perFrameData->AddCommand(cmd);
	}

	return ele;
}

void GRP_Renderer::UpdateRenderer(Transform* targetTrans, Mesh* mesh, ID3D12Device* device)
{
	auto&& ite = dicts.find(targetTrans);
	if (ite == dicts.end()) return;
	RenderElement& rem = elements[ite->second];

	Command cmd;
	cmd.index = ite->second;
	cmd.type = Command::CommandType_Renderer;
	cmd.cmd.objectCBufferAddress = 0;
	cmd.cmd.materialBufferAddress = rem.propertyBuffer.buffer->Resource()->GetGPUVirtualAddress()
		+ rem.propertyBuffer.element * rem.propertyBuffer.buffer->GetAlignedStride();
	cmd.cmd.vertexBuffer = mesh->VertexBufferView();
	cmd.cmd.indexBuffer = mesh->IndexBufferView();
	cmd.cmd.drawArgs.BaseVertexLocation = 0;
	cmd.cmd.drawArgs.IndexCountPerInstance = mesh->GetIndexCount();
	cmd.cmd.drawArgs.InstanceCount = 1;
	cmd.cmd.drawArgs.StartIndexLocation = 0;
	cmd.cmd.drawArgs.StartInstanceLocation = 0;
	for (UINT i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)this->container.GetResource(&FrameResource::mFrameResources[i]->resourceManager, this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(device, capacity);
		});
		perFrameData->AddCommand(cmd);
	}
}

void GRP_Renderer::RemoveElement(Transform* trans, ID3D12Device* device)
{
	auto&& ite = dicts.find(trans);
	if (ite == dicts.end()) return;
	auto arrEnd = elements.end() - 1;
	UINT index = ite->second;
	RenderElement& ele = elements[ite->second];
	pool.Return(ele.propertyBuffer);
	for (UINT i = 0; i < texRequireInMat; ++i)
		textureDescPool.push_back(ele.textures[i]);
	textureIndicesPool.push_back(ele.textures);
	ele = *arrEnd;
	dicts.insert_or_assign(arrEnd->transform.operator->(), ite->second);
	elements.erase(arrEnd);
	dicts.erase(ite);

	Command cmd;
	cmd.index = index;
	cmd.type = Command::CommandType_Remove;
	for (UINT i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)this->container.GetResource(&FrameResource::mFrameResources[i]->resourceManager, this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(device, capacity);
		});
		perFrameData->AddCommand(cmd);
	}

}

void GRP_Renderer::UpdateTransform(Transform* targetTrans, ID3D12Device* device)
{
	auto&& ite = dicts.find(targetTrans);
	if (ite == dicts.end()) return;
	RenderElement& rem = elements[ite->second];
	Command cmd;
	cmd.index = ite->second;
	cmd.type = Command::CommandType_TransformPos;
	cmd.objData.boundingCenter = rem.boundingCenter;
	cmd.objData.boundingExtent = rem.boundingExtent;
	DirectX::XMMATRIX* ptr = (DirectX::XMMATRIX*) &cmd.objData.localToWorld;
	*ptr = targetTrans->GetLocalToWorldMatrix();
	for (UINT i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)this->container.GetResource(&FrameResource::mFrameResources[i]->resourceManager, this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(device, capacity);
		});
		perFrameData->AddCommand(cmd);
	}
}
void  GRP_Renderer::DrawCommand(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	UINT targetShaderPass,
	FrameResource* targetResource,
	ConstBufferElement& cameraProperty,
	ConstBufferElement& cullDataBuffer,
	DirectX::XMFLOAT4* frustumPlanes,
	DirectX::XMFLOAT3 frustumMinPoint,
	DirectX::XMFLOAT3 frustumMaxPoint,
	PSOContainer* container
)
{
	if (elements.size() > cullResultBuffer->GetElementCount(0))
	{
		cullResultBuffer->ReleaseResourceAfterFlush(targetResource);
		StructuredBufferElement ele[2];
		ele[0].elementCount = elements.size();
		ele[0].stride = sizeof(MultiDrawCommand);
		ele[1].elementCount = 1;
		ele[1].stride = sizeof(UINT);
		cullResultBuffer = std::unique_ptr<StructuredBuffer>(
			new StructuredBuffer(
				device,
				ele,
				2,
				true
			));
	}
	cullShader->BindRootSignature(commandList, nullptr);
	UINT dispatchCount = (UINT)ceil(elements.size() / 64.0);
	UINT capacity = this->capacity;
	GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)this->container.GetResource(&targetResource->resourceManager, this, [=]()->GpuDrivenRenderer*
	{
		return new GpuDrivenRenderer(device, capacity);
	});
	CullData cullD;
	cullD._Count = elements.size();
	memcpy(cullD.planes, frustumPlanes, sizeof(DirectX::XMFLOAT4) * 6);
	cullD._FrustumMaxPoint = frustumMaxPoint;
	cullD._FrustumMinPoint = frustumMinPoint;
	cullDataBuffer.buffer->CopyData(cullDataBuffer.element, &cullD);
	cullShader->SetResource(commandList, _InputBuffer, perFrameData->cmdDrawBuffers.get(), 0);
	cullShader->SetResource(commandList, _InputDataBuffer, perFrameData->objectPosBuffer.get(), 0);
	cullShader->SetStructuredBufferByAddress(commandList, _OutputBuffer, cullResultBuffer->GetAddress(0, 0));
	cullShader->SetStructuredBufferByAddress(commandList, _CountBuffer, cullResultBuffer->GetAddress(1, 0));
	cullShader->SetResource(commandList, CBuffer, cullDataBuffer.buffer, cullDataBuffer.element);
	cullShader->Dispatch(commandList, 1, dispatchCount, 1, 1);
	cullShader->Dispatch(commandList, 0, dispatchCount, 1, 1);
	PSODescriptor desc;
	desc.meshLayoutIndex = meshLayoutIndex;
	desc.shaderPass = targetShaderPass;
	desc.shaderPtr = shader;
	ID3D12PipelineState* pso = container->GetState(desc, device);
	commandList->SetPipelineState(pso);
	shader->BindRootSignature(commandList);
	shader->SetResource(commandList, ShaderID::GetMainTex(), textureHeap.operator->(), 0);
	shader->SetResource(commandList, ShaderID::GetPerCameraBufferID(), cameraProperty.buffer, cameraProperty.element);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->ExecuteIndirect(
		cmdSig.GetSignature(),
		elements.size(),
		cullResultBuffer->GetResource(),
		cullResultBuffer->GetAddressOffset(0, 0),
		cullResultBuffer->GetResource(),
		cullResultBuffer->GetAddressOffset(1, 0)
	);
}

CBufferPool* GRP_Renderer::GetCullDataPool(UINT initCapacity)
{
	return new CBufferPool(sizeof(CullData), initCapacity);
}

DescriptorHeap* GRP_Renderer::GetTextureHeap()
{
	return textureHeap.operator->();
}

GRP_Renderer::~GRP_Renderer()
{
	for (UINT i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		container.DisposeResource(&FrameResource::mFrameResources[i]->resourceManager, this);
	}
	delete allocatedIndices;
}
