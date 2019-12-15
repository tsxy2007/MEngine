#pragma once
#include "MObject.h"
#include "../Common/DescriptorHeap.h"
#include "../Common/d3dUtil.h"
class RenderTexture2D : public MObject
{
private:
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthFormat;
	Microsoft::WRL::ComPtr<ID3D12Resource> mResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthResource = nullptr;
	std::unique_ptr<DescriptorHeap> rtvHeap;
	std::unique_ptr<DescriptorHeap> dsvHeap;
	bool isUAV;
public:
	~RenderTexture2D();
	RenderTexture2D(
		ID3D12Device* device,
		UINT width,
		UINT height,
		DXGI_FORMAT format,
		bool useDepth
	);
	void SetUav(bool value, ID3D12GraphicsCommandList* commandList);
	bool GetUav() const;
	void SetViewport(ID3D12GraphicsCommandList* commandList);
	ID3D12Resource* GetDepthResource() const;
	ID3D12Resource* GetColorResource() const;
	DescriptorHeap* GetColorHeap() const;
	DescriptorHeap* GetDepthHeap() const;
	void GetColorViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
	void GetUAVViewDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc);
	void GetDepthViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
	void ClearRenderTarget(ID3D12GraphicsCommandList* commandList, bool clearColor, bool clearDepth);
	DXGI_FORMAT GetColorFormat() const;
	DXGI_FORMAT GetDepthFormat() const;
};