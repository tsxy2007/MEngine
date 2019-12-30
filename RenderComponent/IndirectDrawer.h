#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "UploadBuffer.h"
#include "Shader.h"
#include "Mesh.h"
#include "../Singleton/FrameResource.h"
#include "../Singleton/PSOContainer.h"
#include "CommandSignature.h"
class DescriptorHeap;
struct MeshCommand
{
	Mesh* mesh;
	UINT subMeshIndex;
	UINT materialIndex;
};
struct CullShaderData
{
	UINT _Count;
};
class IndirectDrawer : public MObject
{
private:
	CommandSignature cmdSig;
	Shader* mShader;
	std::vector<MeshCommand> allMeshCommands;
	UploadBuffer materialBuffers;
	UploadBuffer objectBuffers;
	UploadBuffer indirectDataBuffer;
	UploadBuffer csConstBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> indirectDrawBuffer;
public:
	struct HeapSet
	{
		UINT shaderID;
		UINT heapOffset;
	};
	IndirectDrawer(
		Shader* targetShader,
		MeshCommand* commands,
		UINT commandCount,
		size_t objectBufferStride,
		size_t materialBufferStride,
		UINT materialCount,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList
	);
	Shader* GetShader() const { return mShader; }
	void Draw(
		int targetPass,
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		ConstBufferElement* cameraBuffer,
		FrameResource* currentResource,
		PSOContainer* container,
		DescriptorHeap* srvHeap,
		HeapSet* heapSets,
		UINT heapSetCount
	);
	void UploadObjectBuffer(
		const void* dataPtr,
		UINT pos
	);
	void UploadMaterialBuffer(
		const void* dataPtr,
		UINT pos
	);
	UploadBuffer* GetIndirectDataBuffer()
	{
		return &indirectDataBuffer;
	}
	virtual ~IndirectDrawer();
};
