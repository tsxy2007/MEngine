#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "Mesh.h"
#include "Material.h"
#include "CBufferPool.h"
#include "../LogicComponent/Component.h"
class PSOContainer;
class UploadBuffer;
struct IndirectDrawCommand
{
	D3D12_GPU_VIRTUAL_ADDRESS objectCBufferAddress; // Object Constant Buffer Address
	D3D12_VERTEX_BUFFER_VIEW vertexBuffer;			// Vertex Buffer Address
	D3D12_INDEX_BUFFER_VIEW indexBuffer;			//Index Buffer Address
	D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;			//Draw Arguments
};
class MeshRenderer : public Component
{
public:
	virtual ~MeshRenderer();
	ObjectPtr<Mesh> mesh;
	std::vector<ObjectPtr<Material>> mMaterials;
	MeshRenderer(
		Transform* trans,
		ID3D12Device* device,
		ObjectPtr<Mesh>& initMesh,
		std::vector<ObjectPtr<Material>>& allMaterials
	);
	void Draw(
		int targetPass,
		int targetSubMesh,
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		ConstBufferElement* cameraBuffer,
		UploadBuffer* objectBuffer,
		UINT objectBufferOffset,
		PSOContainer* container
	);
	void GetIndirectArgument(
		int targetPass,
		int targetSubMesh,
		ID3D12Device* device,
		UploadBuffer* objectBuffer,
		UINT objectBufferOffset,
		IndirectDrawCommand* command
	);
};