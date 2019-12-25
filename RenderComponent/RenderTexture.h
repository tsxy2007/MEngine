#pragma once
#include "MObject.h"
#include "../Common/d3dUtil.h"
#include "../Common/DescriptorHeap.h"
enum class CubeMapFace : int
{
	PositiveX = 0,
	NegativeX = 1,
	PositiveY = 2,
	NegativeY = 3,
	PositiveZ = 4,
	NegativeZ = 5
};

enum class RenderTextureType : int
{
	Tex2D = 0,
	Tex2DArray = 1,
	Tex3D = 2,
	Cubemap = 3
};

struct RenderTextureDescriptor
{
	UINT width;
	UINT height;
	UINT depthSlice;
	RenderTextureType type;
	DXGI_FORMAT colorFormat;
	DXGI_FORMAT depthFormat;
	bool operator==(const RenderTextureDescriptor& other) const;

	bool operator==(RenderTextureDescriptor&& other) const;

	bool operator!=(const RenderTextureDescriptor& other) const;

	bool operator!=(RenderTextureDescriptor&& other) const;

	//bool operator!=(const)
};

namespace std
{
	template <>
	class hash<RenderTextureDescriptor>
	{
	public:
		size_t operator()(const RenderTextureDescriptor& o) const
		{
			hash<UINT> ulongHash;
			return ulongHash(o.width) ^ ulongHash(o.height) ^ ulongHash(o.depthSlice) ^ ulongHash((UINT)o.type) ^ ulongHash((UINT)o.colorFormat) ^ ulongHash((UINT)o.depthFormat);
		}
	};
}

class RenderTexture : public MObject
{
private:
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
	UINT depthSlice;
	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthFormat;
	Microsoft::WRL::ComPtr<ID3D12Resource> mColorResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthResource;
	DescriptorHeap rtvHeap;
	DescriptorHeap dsvHeap;
	RenderTextureType mType; 
	void GetColorViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
	void GetDepthViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
	void GetColorUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT targetMipLevel);
public:
	virtual ~RenderTexture();
	RenderTextureType GetType() const { return mType; }
	RenderTexture(
		ID3D12Device* device,
		UINT width,
		UINT height,
		DXGI_FORMAT format,
		bool useDepth,
		RenderTextureType type,
		int depthCount,
		int mipCount
	);
	UINT GetWidth() { return mWidth; }
	UINT GetHeight() { return mHeight; }
	void SetViewport(ID3D12GraphicsCommandList* commandList);
	ID3D12Resource* GetDepthResource() const;
	ID3D12Resource* GetColorResource() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetColorDescriptor(UINT slice);
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthDescriptor(UINT slice);
	void BindColorBufferToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device);
	void BindDepthBufferToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device);
	void BindUAVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT targetMipLevel);
	void ClearRenderTarget(ID3D12GraphicsCommandList* commandList, UINT slice, bool clearColor, bool clearDepth);
	DXGI_FORMAT GetColorFormat() const;
	DXGI_FORMAT GetDepthFormat() const;
};