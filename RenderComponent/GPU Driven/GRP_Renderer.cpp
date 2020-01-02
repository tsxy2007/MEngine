#include "GRP_Renderer.h"
#include "../DescriptorHeap.h"
#include "../Mesh.h"
#include "../../LogicComponent/Transform.h"

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
	textureHeap->Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, maxCapacity * texRequireInMat, true);
	for (UINT i = 0, size = textureDescPool.size(); i < size; ++i)
		textureDescPool[i] = i;
	allocatedIndices = new UINT[maxCapacity * texRequireInMat];
	for (UINT i = 0; i < maxCapacity; ++i)
	{
		textureIndicesPool[i] = allocatedIndices + i * texRequireInMat;
	}
}

GRP_Renderer::RenderElement& GRP_Renderer::AddRenderElement(
	ObjectPtr<Transform>& textureHeap,
	ObjectPtr<Mesh>& mesh,
	ID3D12Device* device
)
{
	if (elements.size() >= maxCapacity)
		throw "Out of Range!";
	if (mesh->GetLayoutIndex() != meshLayoutIndex)
		throw "Mesh Bad Layout!";
	dicts.insert_or_assign(std::move(textureHeap.operator->()), elements.size());
	auto&& indPtrIte = textureIndicesPool.end() - 1;
	RenderElement& ele = elements.emplace_back(
		&mesh,
		std::move(pool.GetBuffer(device)),
		&textureHeap,
		*indPtrIte
	);
	textureIndicesPool.erase(indPtrIte);
	for (UINT i = 0; i < texRequireInMat; ++i)
	{
		auto&& ite = textureDescPool.end() - 1;
		ele.textures[i] = *ite;
		textureDescPool.erase(ite);
	}
	return ele;
}

void GRP_Renderer::RemoveElement(Transform* trans)
{
	auto&& ite = dicts.find(trans);
	if (ite == dicts.end()) return;
	auto arrEnd = elements.end() - 1;
	RenderElement& ele = elements[ite->second];
	for (UINT i = 0; i < texRequireInMat; ++i)
		textureDescPool.push_back(ele.textures[i]);
	textureIndicesPool.push_back(ele.textures);
	ele = *arrEnd;
	dicts.insert_or_assign(arrEnd->transform.operator->(), ite->second);
	elements.erase(arrEnd);
	dicts.erase(ite);
}

DescriptorHeap* GRP_Renderer::GetTextureHeap()
{
	return textureHeap.operator->();
}

GRP_Renderer::~GRP_Renderer()
{
	delete allocatedIndices;
}
