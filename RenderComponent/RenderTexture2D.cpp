#include "RenderTexture2D.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;

void RenderTexture2D::ClearRenderTarget(ID3D12GraphicsCommandList* commandList, bool clearColor, bool clearDepth)
{
	if (clearColor) {
		SetUav(false, commandList);
		float colors[4];
		memset(colors, 0, sizeof(float) * 4);
		commandList->ClearRenderTargetView(rtvHeap.hCPU(0), colors, 0, nullptr);
	}
	if (clearDepth && mDepthResource != nullptr)
		commandList->ClearDepthStencilView(dsvHeap.hCPU(0), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0, 0, 0, nullptr);
}

void RenderTexture2D::SetUav(bool value, ID3D12GraphicsCommandList* commandList)
{
	if (value == isUAV) return;
	isUAV = value;
	if (value)
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}
	else
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}
}

bool RenderTexture2D::GetUav() const
{
	return isUAV;
}

ID3D12Resource* RenderTexture2D::GetDepthResource() const
{
	return mDepthResource.Get();
}

ID3D12Resource* RenderTexture2D::GetColorResource() const
{
	return mResource.Get();
}

DescriptorHeap* RenderTexture2D::GetColorHeap()
{
	return &rtvHeap;
}

DescriptorHeap* RenderTexture2D::GetDepthHeap()
{
	return &dsvHeap;
}

void RenderTexture2D::GetColorViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mResource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = mResource->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
}

void RenderTexture2D::GetUAVViewDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT targetMipLevel)
{
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = mResource->GetDesc().Format;;
	uavDesc.Texture2D.MipSlice = targetMipLevel;
	uavDesc.Texture2D.PlaneSlice = 0;
}


void RenderTexture2D::GetDepthViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mDepthResource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = mDepthResource->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
}

RenderTexture2D::RenderTexture2D(
	ID3D12Device* device,
	UINT width,
	UINT height,
	DXGI_FORMAT format,
	UINT mipLevel,
	bool useDepth
) : MObject(),
mWidth(width),
mHeight(height),
mFormat(format),
mDepthFormat(DXGI_FORMAT_D24_UNORM_S8_UINT),
isUAV(false)
{
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = static_cast<float>(width);
	mViewport.Height = static_cast<float>(height);
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;
	mScissorRect = { 0, 0, (LONG)width, (LONG)height };

	rtvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, false);
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = mipLevel;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	D3D12_CLEAR_VALUE rtClear;
	rtClear.Format = mFormat;
	rtClear.Color[0] = 0;
	rtClear.Color[1] = 0;
	rtClear.Color[2] = 0;
	rtClear.Color[3] = 0;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&rtClear,
		IID_PPV_ARGS(&mResource)));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = mFormat;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	device->CreateRenderTargetView(mResource.Get(), &rtvDesc, rtvHeap.hCPU(0));
	if (useDepth)
	{
		dsvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = width;
		depthStencilDesc.Height = height;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;

		// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
		// the depth buffer.  Therefore, because we need to create two views to the same resource:
		//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
		//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
		// we need to create the depth buffer resource with a typeless format.  
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = mDepthFormat;
		optClear.DepthStencil.Depth = 0;
		optClear.DepthStencil.Stencil = 0;
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optClear,
			IID_PPV_ARGS(&mDepthResource)));
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = mDepthFormat;
		dsvDesc.Texture2D.MipSlice = 0;
		device->CreateDepthStencilView(mDepthResource.Get(), &dsvDesc, dsvHeap.hCPU(0));
	}
}

RenderTexture2D::~RenderTexture2D()
{
	mResource = nullptr;
	mDepthResource = nullptr;
}

void RenderTexture2D::SetViewport(ID3D12GraphicsCommandList* commandList)
{
	commandList->RSSetViewports(1, &mViewport);
	commandList->RSSetScissorRects(1, &mScissorRect);
}

DXGI_FORMAT RenderTexture2D::GetColorFormat() const
{
	return mFormat;
}
DXGI_FORMAT RenderTexture2D::GetDepthFormat() const
{
	return mDepthFormat;
}