#pragma once
#include "../Common/d3dUtil.h"
#include "MObject.h"
#include "UploadBuffer.h"
#include "Shader.h"
#include "Mesh.h"
#include "../Singleton/FrameResource.h"
#include "../Singleton/PSOContainer.h"
struct MultiDrawCommand
{
	D3D12_GPU_VIRTUAL_ADDRESS objectCBufferAddress; // Object Constant Buffer Address
	D3D12_GPU_VIRTUAL_ADDRESS materialBufferAddress; // Object Constant Buffer Address
	D3D12_VERTEX_BUFFER_VIEW vertexBuffer;			// Vertex Buffer Address
	D3D12_INDEX_BUFFER_VIEW indexBuffer;			//Index Buffer Address
	D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;			//Draw Arguments
};
struct MeshCommand
{
	Mesh* mesh;
	UINT subMeshIndex;
	UINT materialIndex;
};
class IndirectDrawer : public MObject
{
private:
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> mCommandSignature;
	Shader* mShader;
	std::vector<MeshCommand> allMeshCommands;
	UploadBuffer materialBuffers;
	UploadBuffer objectBuffers;
	UploadBuffer indirectDataBuffer;
public:
	IndirectDrawer(
		Shader* targetShader,
		MeshCommand* commands,
		size_t objectBufferStride,
		UINT commandCount,
		size_t materialBufferStride,
		UINT materialCount,
		ID3D12Device* device
	);
	void Draw(
		int targetPass,
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		ConstBufferElement* cameraBuffer,
		FrameResource* currentResource,
		PSOContainer* container
	);
};