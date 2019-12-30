#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "Mesh.h"
#include "Material.h"
#include "CBufferPool.h"
#include "../LogicComponent/Component.h"
class FrameResource;
class PSOContainer;
struct IndirectDrawCommand
{
	D3D12_GPU_VIRTUAL_ADDRESS objectCBufferAddress; // Object Constant Buffer Address
	D3D12_VERTEX_BUFFER_VIEW vertexBuffer;			// Vertex Buffer Address
	D3D12_INDEX_BUFFER_VIEW indexBuffer;			//Index Buffer Address
	D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;			//Draw Arguments
};
class MeshRenderer : public Component
{
private:
	static CBufferPool objectPool;
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
		FrameResource* currentResource,
		PSOContainer* container
	);
	void GetIndirectArgument(
		int targetPass,
		int targetSubMesh,
		ID3D12Device* device,
		FrameResource* currentResource,
		IndirectDrawCommand* command
	);
	void UpdateObjectBuffer(FrameResource* resource);
};