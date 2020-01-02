#pragma once
#include "../../Common/MObject.h"
#include "../CBufferPool.h"
#include "../CommandSignature.h"
#include "../../Common/MetaLib.h"
class DescriptorHeap;
class Mesh;
class Transform;
class FrameResource;
class ComputeShader;
class GRP_Renderer : public MObject
{
private:
	struct RenderElement
	{
		ObjectPtr<Mesh> mesh;
		ConstBufferElement propertyBuffer;
		ObjectPtr<Transform> transform;
		UINT* textures;
		RenderElement(
			ObjectPtr<Mesh>* anotherMesh,
			ConstBufferElement&& anotherBuffer,
			ObjectPtr<Transform>* anotherTrans,
			UINT* textures
		) : mesh(*anotherMesh), propertyBuffer(anotherBuffer), transform(*anotherTrans), textures(textures) {}
	};
	CBufferPool pool;
	ObjectPtr<DescriptorHeap> textureHeap;
	size_t cbufferStride;
	CommandSignature cmdSig;
	Shader* shader;
	UINT maxCapacity;
	std::vector<RenderElement> elements;
	std::unordered_map<Transform*, UINT> dicts;
	std::vector<UINT> textureDescPool;
	std::vector<UINT*> textureIndicesPool;
	UINT* allocatedIndices;
	UINT texRequireInMat;
	UINT meshLayoutIndex;
	ComputeShader* cullShader;
public:
	GRP_Renderer(
		size_t materialPropertyStride,
		UINT meshLayoutIndex,
		UINT initCapacity,
		UINT maxCapacity,
		Shader* shader,
		UINT texRequireInMat,
		ID3D12Device* device
	);
	~GRP_Renderer();
	RenderElement& AddRenderElement(
		ObjectPtr<Transform>& textureHeap,
		ObjectPtr<Mesh>& mesh,
		ID3D12Device* device
	);

	void RemoveElement(Transform* trans);
	
	CommandSignature* GetCmdSignature() { return &cmdSig; }
	DescriptorHeap* GetTextureHeap();
	void UpdateTransform(Transform* targetTrans);
	void DrawCommand(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		UINT targetShaderPass,
		FrameResource* targetResource,
		ConstBufferElement& cameraProperty,
		UploadBuffer* cullDataBuffer
	);
};

