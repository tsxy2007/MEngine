#include "RenderTextureCube.h"
RenderTextureCube::~RenderTextureCube()
{
	mColorResource = nullptr;
	mDepthResource = nullptr;
}
ID3D12Resource* RenderTextureCube::GetDepthResource() const { return mDepthResource.Get(); }
ID3D12Resource* RenderTextureCube::GetColorResource() const { return mColorResource.Get(); }
DXGI_FORMAT RenderTextureCube::GetColorFormat() const { return mFormat; }
DXGI_FORMAT RenderTextureCube::GetDepthFormat() const { return mDepthFormat; }
void RenderTextureCube::ClearRenderTarget(ID3D12GraphicsCommandList* commandList, CubeMapFace face, DirectX::XMVECTORF32 color, bool clearColor, bool clearDepth)
{
	if (clearColor)
		commandList->ClearRenderTargetView(rtvHeap.hCPU((int)face), color, 0, nullptr);
	if (clearDepth && mDepthResource != nullptr)
		commandList->ClearDepthStencilView(dsvHeap.hCPU((int)face), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0, 0, 0, nullptr);
}
void RenderTextureCube::GetDepthViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mDepthResource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = mDepthResource->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
}

void RenderTextureCube::SetViewport(ID3D12GraphicsCommandList* commandList)
{
	commandList->RSSetViewports(1, &mViewport);
	commandList->RSSetScissorRects(1, &mScissorRect);
}

void RenderTextureCube::GetColorViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mColorResource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = mColorResource->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
}
D3D12_CPU_DESCRIPTOR_HANDLE RenderTextureCube::GetColorDescriptor(CubeMapFace face)
{
	return rtvHeap.hCPU((int)face);
}
D3D12_CPU_DESCRIPTOR_HANDLE RenderTextureCube::GetDepthDescriptor(CubeMapFace face)
{
	return dsvHeap.hCPU((int)face);
}

RenderTextureCube::RenderTextureCube(
	ID3D12Device* device,
	UINT width,
	UINT height,
	DXGI_FORMAT format,
	bool useDepth,
	int mipCount
) : MObject(),
	mWidth(width),
	mHeight(height),
	mFormat(format),
	mViewport({ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f }),
	mScissorRect({ 0, 0, (int)width, (int)height }),
	mDepthFormat(DXGI_FORMAT_D24_UNORM_S8_UINT)
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 6;
	texDesc.MipLevels = mipCount;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr,
		IID_PPV_ARGS(mColorResource.GetAddressOf())));

	rtvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 6, false);
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Format = mFormat;
	rtvDesc.Texture2DArray.MipSlice = 0;
	rtvDesc.Texture2DArray.PlaneSlice = 0;
	rtvDesc.Texture2DArray.ArraySize = 1;

	for (int i = 0; i < 6; ++i)
	{

		// Render target to ith element.
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		// Create RTV to ith cubemap face.
		device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, rtvHeap.hCPU(i));
	}
	if (useDepth)
	{
		dsvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 6, false);
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = width;
		depthStencilDesc.Height = height;
		depthStencilDesc.DepthOrArraySize = 6;
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
			IID_PPV_ARGS(mDepthResource.GetAddressOf())));
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Format = mDepthFormat;
		dsvDesc.Texture2DArray.ArraySize = 1;
		dsvDesc.Texture2DArray.MipSlice = 0;
		for (int i = 0; i < 6; ++i)
		{
			dsvDesc.Texture2DArray.FirstArraySlice = i;
			device->CreateDepthStencilView(mDepthResource.Get(), &dsvDesc, dsvHeap.hCPU(i));
		}
	}
}