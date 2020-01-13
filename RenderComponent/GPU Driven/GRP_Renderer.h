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
class CBufferPool;
class PSOContainer;
class StructuredBuffer;
class GRP_Renderer : public MObject
{
public:
	struct CullData
	{
		DirectX::XMFLOAT4 planes[6];
		//Align
		DirectX::XMFLOAT3 _FrustumMinPoint;
		UINT _Count;
		//Align
		DirectX::XMFLOAT3 _FrustumMaxPoint;
	};
	struct RenderElement
	{
		ConstBufferElement propertyBuffer;
		ObjectPtr<Transform> transform;
		UINT* textures;
		DirectX::XMFLOAT3 boundingCenter;
		DirectX::XMFLOAT3 boundingExtent;
		RenderElement(
			ConstBufferElement&& anotherBuffer,
			ObjectPtr<Transform>* anotherTrans,
			UINT* textures,
			DirectX::XMFLOAT3 boundingCenter,
			DirectX::XMFLOAT3 boundingExtent
		) : propertyBuffer(anotherBuffer), transform(*anotherTrans), textures(textures), boundingCenter(boundingCenter), boundingExtent(boundingExtent) {}
	};
private:
	CBufferPool pool;
	ObjectPtr<DescriptorHeap> textureHeap;
	size_t cbufferStride;
	CommandSignature cmdSig;
	Shader* shader;
	UINT maxCapacity;
	std::unique_ptr<StructuredBuffer> cullResultBuffer;
	std::vector<RenderElement> elements;
	std::unordered_map<Transform*, UINT> dicts;
	std::vector<UINT> textureDescPool;
	std::vector<UINT*> textureIndicesPool;
	UINT* allocatedIndices;
	UINT texRequireInMat;
	UINT meshLayoutIndex;
	UINT capacity;
	UINT _InputBuffer;
	UINT _InputDataBuffer;
	UINT _OutputBuffer;
	UINT _CountBuffer;
	UINT CullBuffer;
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
		ObjectPtr<Transform>& targetTrans,
		ObjectPtr<Mesh>& mesh,
		ID3D12Device* device
	);
	static CBufferPool* GetCullDataPool(UINT initCapacity);
	void RemoveElement(Transform* trans, ID3D12Device* device);
	void UpdateRenderer(Transform* targetTrans, Mesh* mesh, ID3D12Device* device);
	CommandSignature* GetCmdSignature() { return &cmdSig; }
	DescriptorHeap* GetTextureHeap();
	void UpdateFrame(FrameResource*, ID3D12Device*);//Should be called Per frame
	void UpdateTransform(Transform* targetTrans, ID3D12Device* device);
	void DrawCommand(
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
	);
};
