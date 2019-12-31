#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Singleton/MeshLayout.h"
struct SubMesh
{
	DXGI_FORMAT indexFormat;
	int indexCount;
	void* indexArrayPtr;
};
class Mesh : public MObject
{
	Microsoft::WRL::ComPtr<ID3D12Resource> dataBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = nullptr;

	// Data about the buffers.
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	UINT meshLayoutIndex;
	UINT mVertexCount;
	char* dataPtr = nullptr;
	std::array<int, 8> offsets;
	std::vector<size_t> indexOffsets;
	std::vector<SubMesh> mSubMeshes;
	DirectX::XMFLOAT3 boundingCenter;
	DirectX::XMFLOAT3 boundingExtent;
public:
	virtual ~Mesh();
	inline UINT GetLayoutIndex() const { return meshLayoutIndex; }
	inline UINT GetSubmeshSize() const { return mSubMeshes.size(); }
	inline SubMesh& GetSubmesh(int i) { return mSubMeshes[i]; }
	inline size_t GetSubmeshByteOffset(int i) const { return indexOffsets[i]; }
	inline UINT GetVertexCount() const { return mVertexCount; }
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView(int submesh);
	Mesh(
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
		SubMesh* subMeshes,
		UINT subMeshCount
	);
};