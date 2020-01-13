#pragma once
#include "../Common/MObject.h"
#include "../Common/d3dUtil.h"
#include "../RenderComponent/DescriptorHeap.h"
enum CubeMapFace
{
	CubeMapFace_PositiveX = 0,
	CubeMapFace_NegativeX = 1,
	CubeMapFace_PositiveY = 2,
	CubeMapFace_NegativeY = 3,
	CubeMapFace_PositiveZ = 4,
	CubeMapFace_NegativeZ = 5
};

enum RenderTextureType
{
	RenderTextureType_Tex2D = 0,
	RenderTextureType_Tex2DArray = 1,
	RenderTextureType_Tex3D = 2,
	RenderTextureType_Cubemap = 3
};
enum RenderTextureDepthSettings
{
	RenderTextureDepthSettings_None,
	RenderTextureDepthSettings_Depth16,
	RenderTextureDepthSettings_Depth32,
	RenderTextureDepthSettings_DepthStencil
};
struct RenderTextureDescriptor
{
	UINT width;
	UINT height;
	UINT depthSlice;
	RenderTextureType type;
	DXGI_FORMAT colorFormat;
	RenderTextureDepthSettings depthType;
	bool operator==(const RenderTextureDescriptor& other) const;

	bool operator!=(const RenderTextureDescriptor& other) const;
	//bool operator!=(const)
};

class RenderTexture : public MObject
{
private:
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
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
		RenderTextureDepthSettings depthByte,
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
	void BindColorBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device);
	void BindDepthBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device);
	void BindUAVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT targetMipLevel);
	void ClearRenderTarget(ID3D12GraphicsCommandList* commandList, UINT slice, bool clearColor, bool clearDepth);
	DXGI_FORMAT GetColorFormat() const;
	DXGI_FORMAT GetDepthFormat() const;
};