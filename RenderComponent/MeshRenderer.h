#pragma once
#include "../Common/d3dUtil.h"
#include "MObject.h"
#include "Mesh.h"
#include "Material.h"
#include "../Singleton/PSOContainer.h"
#include "../Singleton/ShaderID.h"
#include "CBufferPool.h"
#include "../Singleton/FrameResource.h"
#include "../LogicComponent/Transform.h"
struct IndirectDrawCommand
{
	D3D12_GPU_VIRTUAL_ADDRESS objectCBufferAddress; // Object Constant Buffer Address
	D3D12_VERTEX_BUFFER_VIEW vertexBuffer;			// Vertex Buffer Address
	D3D12_INDEX_BUFFER_VIEW indexBuffer;			//Index Buffer Address
	D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;			//Draw Arguments
};
class MeshRenderer : public MObject
{
private:
	static CBufferPool objectPool;
public:
	virtual ~MeshRenderer();
	ObjectPtr<Mesh> mesh;
	std::vector<ObjectPtr<Material>> mMaterials;
	Transform transform;
	MeshRenderer(
		ID3D12Device* device,
		DirectX::XMFLOAT3 initPosition,
		DirectX::XMVECTOR initQuaternion,
		DirectX::XMFLOAT3 localScale,
		ObjectPtr<Mesh> initMesh,
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