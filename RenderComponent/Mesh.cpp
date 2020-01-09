#include "Mesh.h"
#include "../Singleton/FrameResource.h"
#include "../Singleton/MeshLayout.h"
//CPP
D3D12_VERTEX_BUFFER_VIEW Mesh::VertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = dataBuffer->GetGPUVirtualAddress();
	vbv.StrideInBytes = VertexByteStride;
	vbv.SizeInBytes = VertexBufferByteSize;

	return vbv;
}


D3D12_INDEX_BUFFER_VIEW Mesh::IndexBufferView()
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = dataBuffer->GetGPUVirtualAddress() + VertexBufferByteSize;
	ibv.Format = indexFormat;
	ibv.SizeInBytes = indexCount * ((indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4);
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
	DXGI_FORMAT indexFormat,
	UINT indexCount,
	void* indexArrayPtr,
	FrameResource* res
) : MObject(), dataPtr(nullptr), mVertexCount(vertexCount),
indexFormat(indexFormat),
indexCount(indexCount),
indexArrayPtr(indexArrayPtr)
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
	size_t indexSize = indexCount * ((indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4);
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
	memcpy(indexBufferStart, indexArrayPtr, indexCount * ((indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4));
	dataBuffer = d3dUtil::CreateDefaultBuffer(device, commandList, dataPtr, indexSize + VertexBufferByteSize, uploadBuffer);
	FrameResource::mCurrFrameResource->ReleaseResourceAfterFlush(uploadBuffer, res);
}


struct CharPart
{
	char* ptr;
	size_t start;
	size_t size;
	bool operator==(std::string& str)
	{
		if (str.size() != size) return false;
		for (size_t i = 0; i < size; ++i)
		{
			if (str[i] != ptr[start + i]) return false;
		}
		return true;
	}
	bool operator!= (std::string& str)
	{
		return !operator==(str);
	}
	bool operator==(std::string&& str)
	{
		if (str.size() != size) return false;
		for (size_t i = 0; i < size; ++i)
		{
			if (str[i] != ptr[start + i]) return false;
		}
		return true;
	}
	bool operator!= (std::string&& str)
	{
		return !operator==(std::move(str));
	}

	bool CheckPartialEqual(std::string& str, size_t startPos, size_t endPos)
	{
		for (size_t i = startPos; i < endPos; ++i)
		{
			if (ptr[i + start] != str[i]) return false;
		}
		return true;
	}
	bool CheckPartialEqual(std::string&& str, size_t startPos, size_t endPos)
	{
		for (size_t i = startPos; i < endPos; ++i)
		{
			if (ptr[i + start] != str[i]) return false;
		}
		return true;
	}
	UINT ToUInt()
	{
		UINT times = 1;
		UINT value = 0;
		int v = start;
		for (int end = start + size - 1; end >= v; --end)
		{
			value += ((UINT)ptr[end] - 48) * times;
			times *= 10;
		}
		return value;
	}
	UINT ToUInt(int start, int end)
	{
		UINT times = 1;
		UINT value = 0;
		for (int i = end - 1; i >= start; --i)
		{
			value += ((UINT)ptr[i] - 48) * times;
			times *= 10;
		}
		return value;
	}
};

void SplitString(char* targetString, size_t size, std::vector<CharPart>& vec, const char splitFlag = ' ')
{
	vec.clear();
	size_t startPos = 0;
	for (size_t i = 0; i < size; ++i)
	{
		char c = targetString[i];
		size_t splitSize = i - startPos;
		if (c == splitFlag && splitSize > 0)
		{
			vec.emplace_back<CharPart>({ targetString, startPos, splitSize });
			startPos = i + 1;
		}
	}
	size_t splitSize = size - startPos;
	if (splitSize > 0)
	{
		vec.emplace_back<CharPart>({ targetString, startPos, splitSize });
	}
}
using namespace DirectX;
UINT DecodeMesh(
	std::string& meshPath,
	DirectX::XMFLOAT3*& positions,
	DirectX::XMFLOAT3*& normals,
	DirectX::XMFLOAT4*& tangents,
	DirectX::XMFLOAT4*& colors,
	DirectX::XMFLOAT2*& uv,
	DirectX::XMFLOAT2*& uv2,
	DirectX::XMFLOAT2*& uv3,
	DirectX::XMFLOAT2*& uv4,
	std::vector<std::vector<UINT>>& subMeshes)
{
	std::ifstream input(meshPath);
	char sign[32];
	std::vector<CharPart> commandParts(5);
	UINT index = 0;
	positions = nullptr;
	normals = nullptr;
	tangents = nullptr;
	uv = nullptr;
	uv2 = nullptr;
	uv3 = nullptr;
	uv4 = nullptr;
	colors = nullptr;

	while (true)
	{
		input.getline((char*)sign, 32);
		UINT commandLength = strlen((char*)sign);
		if (commandLength == 0)
			return index;
		SplitString(sign, commandLength, commandParts);
		CharPart pt = commandParts[0];
		CharPart sizeCount = commandParts[1];
		UINT dataSize = sizeCount.ToUInt();
		UINT size = sizeCount.size;
		if (pt.CheckPartialEqual("sub", 0, 3))
		{
			UINT subMeshIndex = pt.ToUInt(3, pt.size);
			if ((1 + subMeshIndex) > subMeshes.size())
				subMeshes.resize(subMeshIndex + 1);
			std::vector<UINT>& vec = subMeshes[subMeshIndex];
			UINT triCount = dataSize / sizeof(int);
			vec.resize(triCount);
			input.read((char*)vec.data(), dataSize);
		}
		else if (pt == "v")
		{
			UINT vertCount = dataSize / sizeof(DirectX::XMFLOAT3);
			index = vertCount;
			positions = new DirectX::XMFLOAT3[vertCount];
			input.read((char*)positions, dataSize);
		}
		else if (pt == "n")
		{
			//Normal
			UINT normalCount = dataSize / sizeof(DirectX::XMFLOAT3);
			normals = new DirectX::XMFLOAT3[normalCount];
			input.read((char*)normals, dataSize);
		}
		else if (pt == "t")
		{
			//Tangent
			UINT tangentCount = dataSize / sizeof(DirectX::XMFLOAT4);

			tangents = new DirectX::XMFLOAT4[tangentCount];
			input.read((char*)tangents, dataSize);
		}
		else if (pt == "uv1")
		{
			UINT uvCount = dataSize / sizeof(DirectX::XMFLOAT2);
			uv = new DirectX::XMFLOAT2[uvCount];
			input.read((char*)uv, dataSize);
		}
		else if (pt == "uv2")
		{
			UINT uvCount = dataSize / sizeof(DirectX::XMFLOAT2);
			uv2 = new DirectX::XMFLOAT2[uvCount];
			input.read((char*)uv2, dataSize);
		}
		else if (pt == "uv3")
		{
			UINT uvCount = dataSize / sizeof(DirectX::XMFLOAT2);
			uv3 = new DirectX::XMFLOAT2[uvCount];
			input.read((char*)uv3, dataSize);
		}
		else if (pt == "uv4")
		{
			UINT uvCount = dataSize / sizeof(DirectX::XMFLOAT2);
			uv4 = new DirectX::XMFLOAT2[uvCount];
			input.read((char*)uv4, dataSize);
		}
		else if (pt == "color")
		{
			UINT colorCount = dataSize / sizeof(DirectX::XMFLOAT4);
			colors = new DirectX::XMFLOAT4[colorCount];
			input.read((char*)colors, dataSize);
		}
		input.get();// \n
	}
	return index;
}