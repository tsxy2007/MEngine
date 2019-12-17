#include "Mesh.h"
#include "../Singleton/FrameResource.h"
//CPP
D3D12_VERTEX_BUFFER_VIEW Mesh::VertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = dataBuffer->GetGPUVirtualAddress();
	vbv.StrideInBytes = VertexByteStride;
	vbv.SizeInBytes = VertexBufferByteSize;

	return vbv;
}


D3D12_INDEX_BUFFER_VIEW Mesh::IndexBufferView(int submesh) const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = dataBuffer->GetGPUVirtualAddress() + VertexBufferByteSize +(*indexOffsets)[submesh];
	SubMesh& subm = (*mSubMeshes)[submesh];
	ibv.Format = subm.indexFormat;
	ibv.SizeInBytes = subm.indexCount * ((subm.indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4);
	return ibv;
}

Mesh::~Mesh()
{
	dataBuffer = nullptr;
	if (dataPtr != nullptr)
	{
		delete dataPtr;
		dataPtr = nullptr;
	}
	if (indexOffsets != nullptr)
	{
		delete indexOffsets;
		indexOffsets = nullptr;
	}
	if (mSubMeshes != nullptr)
	{
		delete mSubMeshes;
		mSubMeshes = nullptr;
	}
}


Mesh::Mesh(
	int vertexCount,
	DirectX::XMFLOAT3* positions,
	DirectX::XMFLOAT3* normals,
	DirectX::XMFLOAT4* tangents,
	DirectX::XMFLOAT4* colors,
	DirectX::XMFLOAT2* uv,
	DirectX::XMFLOAT2* uv1,
	DirectX::XMFLOAT2* uv2,
	DirectX::XMFLOAT2* uv3,
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	std::vector<SubMesh>* subMeshes
) : dataPtr(nullptr), mVertexCount(vertexCount), mSubMeshes(subMeshes)
{

	//IndexFormat = indexFormat == DXGI_FORMAT_R16_UINT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	//TODO
	meshLayoutIndex = MeshLayout::GetMeshLayoutIndex(
		normals != nullptr,
		tangents != nullptr,
		colors != nullptr,
		uv != nullptr,
		uv1 != nullptr,
		uv2 != nullptr,
		uv3 != nullptr
	);
	std::vector<D3D12_INPUT_ELEMENT_DESC>* meshLayouts = MeshLayout::GetMeshLayoutValue(meshLayoutIndex);
	UINT stride = 0;
	auto cumulate = [&](void* ptr, UINT size) -> void
	{
		if (ptr != nullptr) stride += size;
	};
	cumulate(positions, 12);
	cumulate(normals, 12);
	cumulate(tangents, 16);
	cumulate(colors, 16);
	cumulate(uv, 8);
	cumulate(uv1, 8);
	cumulate(uv2, 8);
	cumulate(uv3, 8);
	VertexByteStride = stride;
	VertexBufferByteSize = stride * vertexCount;
	//IndexBufferByteSize = (IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4) * indexCount;
	size_t indexSize = 0;
	indexOffsets = new std::vector<size_t>(subMeshes->size());
	for (int i = 0; i < subMeshes->size(); ++i) {
		(*indexOffsets)[i] = indexSize;
		indexSize += (*subMeshes)[i].indexCount * (((*subMeshes)[i].indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4);
	}
	dataPtr = reinterpret_cast<char*>(malloc(VertexBufferByteSize + indexSize));

	auto vertBufferCopy = [&](char* buffer, char* ptr, UINT size, int& offset, std::array<int, 8>& offstArray, int arrayLen) -> void
	{
		if (ptr == nullptr)
		{
			offstArray[arrayLen] = -1;
			return;
		}
		for (int i = 0; i < vertexCount; ++i)
		{
			memcpy(buffer + i * stride + offset, ptr + size * i, size);
		}
		offstArray[arrayLen] = offset;
		offset += size;
	};
	int offset = 0;
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(positions),
		12,
		offset,
		offsets,
		0
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(normals),
		12,
		offset,
		offsets,
		1
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(tangents),
		16,
		offset,
		offsets,
		2
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(colors),
		16,
		offset,
		offsets,
		3
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv),
		8,
		offset,
		offsets,
		4
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv1),
		8,
		offset,
		offsets,
		5
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv2),
		8,
		offset,
		offsets,
		6
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv3),
		8,
		offset,
		offsets,
		7
	);
	char* indexBufferStart = dataPtr + VertexBufferByteSize;
	for (int i = 0; i < subMeshes->size(); ++i)
	{
		memcpy(indexBufferStart + (*indexOffsets)[i], (*subMeshes)[i].indexArrayPtr, (*subMeshes)[i].indexCount * (((*subMeshes)[i].indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4));
	}
	dataBuffer = d3dUtil::CreateDefaultBuffer(device, commandList, dataPtr, indexSize + VertexBufferByteSize, uploadBuffer);
	FrameResource::mCurrFrameResource->ReleaseResourceAfterFlush(uploadBuffer);
}