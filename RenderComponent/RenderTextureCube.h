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

class RenderTextureCube : MObject
{
private:
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthFormat;
	Microsoft::WRL::ComPtr<ID3D12Resource> mColorResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthResource;
	DescriptorHeap rtvHeap;
	DescriptorHeap dsvHeap;
public:
	virtual ~RenderTextureCube();
	RenderTextureCube(
		ID3D12Device* device,
		UINT width,
		UINT height,
		DXGI_FORMAT format,
		bool useDepth,
		int mipCount
	);
	void SetViewport(ID3D12GraphicsCommandList* commandList);
	ID3D12Resource* GetDepthResource() const;
	ID3D12Resource* GetColorResource() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetColorDescriptor(CubeMapFace face);
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthDescriptor(CubeMapFace face);
	void GetColorViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
	void GetDepthViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
	void ClearRenderTarget(ID3D12GraphicsCommandList* commandList, CubeMapFace face, DirectX::XMVECTORF32 color, bool clearColor, bool clearDepth);
	DXGI_FORMAT GetColorFormat() const;
	DXGI_FORMAT GetDepthFormat() const;
};