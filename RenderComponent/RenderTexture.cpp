#include "RenderTexture.h"
RenderTexture::~RenderTexture()
{
	mColorResource = nullptr;
	mDepthResource = nullptr;
}
ID3D12Resource* RenderTexture::GetDepthResource() const { return mDepthResource.Get(); }
ID3D12Resource* RenderTexture::GetColorResource() const { return mColorResource.Get(); }
DXGI_FORMAT RenderTexture::GetColorFormat() const { return mFormat; }
DXGI_FORMAT RenderTexture::GetDepthFormat() const { return mDepthFormat; }
void RenderTexture::ClearRenderTarget(ID3D12GraphicsCommandList* commandList, UINT slice, bool clearColor, bool clearDepth)
{
	if (clearColor)
	{
		float colors[4] = { 0,0,0,0 };
		commandList->ClearRenderTargetView(rtvHeap.hCPU(slice), colors, 0, nullptr);
	}
	if (clearDepth && mDepthResource != nullptr)
		commandList->ClearDepthStencilView(dsvHeap.hCPU(slice), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0, 0, 0, nullptr);
}

bool RenderTextureDescriptor::operator==(const RenderTextureDescriptor& other) const
{
	return
		width == other.width &&
		height == other.height &&
		depthSlice == other.depthSlice &&
		type == other.type &&
		colorFormat == other.colorFormat &&
		depthType == other.depthType;
}

bool RenderTextureDescriptor::operator==(RenderTextureDescriptor&& other) const
{
	return
		width == other.width &&
		height == other.height &&
		depthSlice == other.depthSlice &&
		type == other.type &&
		colorFormat == other.colorFormat &&
		depthType == other.depthType;
}

bool RenderTextureDescriptor::operator!=(const RenderTextureDescriptor& other) const
{
	return
		width != other.width &&
		height != other.height &&
		depthSlice != other.depthSlice &&
		type != other.type &&
		colorFormat != other.colorFormat &&
		depthType != other.depthType;
}

bool RenderTextureDescriptor::operator!=(RenderTextureDescriptor&& other) const
{
	return
		width != other.width &&
		height != other.height &&
		depthSlice != other.depthSlice &&
		type != other.type &&
		colorFormat != other.colorFormat &&
		depthType != other.depthType;
}
void RenderTexture::GetDepthViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mDepthResource->GetDesc().Format;
	switch (mType)
	{
	case RenderTextureType::RenderTextureType_Cubemap:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = mDepthResource->GetDesc().MipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case RenderTextureType::RenderTextureType_Tex2D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = mDepthResource->GetDesc().MipLevels;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case RenderTextureType::RenderTextureType_Tex2DArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = mDepthResource->GetDesc().MipLevels;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2DArray.ArraySize = depthSlice;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		break;
	case RenderTextureType::RenderTextureType_Tex3D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MipLevels = mDepthResource->GetDesc().MipLevels;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
		break;
	}
	
}

void RenderTexture::SetViewport(ID3D12GraphicsCommandList* commandList)
{
	commandList->RSSetViewports(1, &mViewport);
	commandList->RSSetScissorRects(1, &mScissorRect);
}

void RenderTexture::GetColorUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT targetMipLevel)
{
	UINT maxLevel = mColorResource->GetDesc().MipLevels - 1;
	targetMipLevel = min(targetMipLevel, maxLevel);
	uavDesc.Format = mColorResource->GetDesc().Format;
	switch (mType)
	{
	case RenderTextureType::RenderTextureType_Tex2D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = targetMipLevel;
		uavDesc.Texture2D.PlaneSlice = 0;
		break;
	case RenderTextureType::RenderTextureType_Tex3D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.MipSlice = targetMipLevel;
		uavDesc.Texture3D.WSize = depthSlice;
		break;
	default:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = depthSlice;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.MipSlice = targetMipLevel;
		uavDesc.Texture2DArray.PlaneSlice = 0;
		break;
	}
}

void RenderTexture::GetColorViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mColorResource->GetDesc().Format;
	switch (mType)
	{
	case RenderTextureType::RenderTextureType_Cubemap:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = mColorResource->GetDesc().MipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case RenderTextureType::RenderTextureType_Tex2D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = mColorResource->GetDesc().MipLevels;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case RenderTextureType::RenderTextureType_Tex2DArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = mColorResource->GetDesc().MipLevels;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2DArray.ArraySize = depthSlice;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		break;
	case RenderTextureType::RenderTextureType_Tex3D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MipLevels = mColorResource->GetDesc().MipLevels;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
		break;
	}
}
D3D12_CPU_DESCRIPTOR_HANDLE RenderTexture::GetColorDescriptor(UINT slice)
{
	return rtvHeap.hCPU(slice);
}
D3D12_CPU_DESCRIPTOR_HANDLE RenderTexture::GetDepthDescriptor(UINT slice)
{
	return dsvHeap.hCPU(slice);
}
/*
void RenderTexture::SetUAV(ID3D12GraphicsCommandList* commandList, bool value)
{
	CD3DX12_RESOURCE_BARRIER result;
	ZeroMemory(&result, sizeof(result));
	D3D12_RESOURCE_BARRIER &barrier = result;
	result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	result.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = mColorResource.Get();

	if (value)
		commandList->ResourceBarrier(1, &result);
	else
		commandList->ResourceBarrier(1, &result);
}
*/

void RenderTexture::BindRTVToHeap(ID3D12Device* device, DescriptorHeap* targetHeap, UINT targetHeapIndex, UINT slice)
{
	UINT maxDepth = depthSlice - 1;
	slice = min(slice, maxDepth);
	switch (mType)
	{
	case RenderTextureType::RenderTextureType_Tex2D:

		device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, targetHeap->hCPU(targetHeapIndex));
		break;

	case RenderTextureType::RenderTextureType_Tex3D:
		rtvDesc.Texture3D.FirstWSlice = slice;
		device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, targetHeap->hCPU(targetHeapIndex));
		break;
	default:
		rtvDesc.Texture2DArray.FirstArraySlice = slice;
		device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, targetHeap->hCPU(targetHeapIndex));
	}
}

RenderTexture::RenderTexture(
	ID3D12Device* device,
	UINT width,
	UINT height,
	DXGI_FORMAT format,
	RenderTextureDepthSettings depthByte,
	RenderTextureType type,
	int depthCount,
	int mipCount
) : MObject(),
mType(type),
mWidth(width),
mHeight(height),
mFormat(format),
mViewport({ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f }),
mScissorRect({ 0, 0, (int)width, (int)height })
{
	UINT arraySize;
	switch (type)
	{
	case RenderTextureType::RenderTextureType_Cubemap:
		arraySize = 6;
		break;
	case RenderTextureType::RenderTextureType_Tex2D:
		arraySize = 1;
		break;
	default:
		arraySize = max(1, depthCount);
		break;
	}
	mipCount = max(1, mipCount);
	depthSlice = arraySize;
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = (type == RenderTextureType::RenderTextureType_Tex3D) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = arraySize;
	texDesc.MipLevels = mipCount;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr,
		IID_PPV_ARGS(&mColorResource)));

	switch (type)
	{
	case RenderTextureType::RenderTextureType_Tex2D:
		rtvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, arraySize, false);
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = mFormat;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, rtvHeap.hCPU(0));
		break;

	case RenderTextureType::RenderTextureType_Tex3D:
		rtvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, arraySize, false);
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		rtvDesc.Format = mFormat;
		rtvDesc.Texture3D.MipSlice = 0;
		rtvDesc.Texture3D.WSize = arraySize;
		for (int i = 0; i < arraySize; ++i)
		{
			// Render target to ith element.
			rtvDesc.Texture3D.FirstWSlice = i;
			// Create RTV to ith cubemap face.
			device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, rtvHeap.hCPU(i));
		}
		break;
	default:
		rtvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, arraySize, false);
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = mFormat;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.ArraySize = 1;
		for (int i = 0; i < arraySize; ++i)
		{
			// Render target to ith element.
			rtvDesc.Texture2DArray.FirstArraySlice = i;
			// Create RTV to ith cubemap face.
			device->CreateRenderTargetView(mColorResource.Get(), &rtvDesc, rtvHeap.hCPU(i));
		}
		break;
	}
	DXGI_FORMAT mResourceFormat;
	switch (depthByte)
	{
	case RenderTextureDepthSettings_Depth32:
		mResourceFormat = DXGI_FORMAT_R32_FLOAT;
		mDepthFormat = DXGI_FORMAT_D32_FLOAT;
		break;
	case RenderTextureDepthSettings_Depth16:
		mResourceFormat = DXGI_FORMAT_R16_UNORM;
		mDepthFormat = DXGI_FORMAT_D16_UNORM;
		break;
	case RenderTextureDepthSettings_DepthStencil:
		mResourceFormat = DXGI_FORMAT_R24G8_TYPELESS;
		mDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	default:
		mResourceFormat = DXGI_FORMAT_UNKNOWN;
		mDepthFormat = DXGI_FORMAT_UNKNOWN;
		break;
	}

	if (depthByte != RenderTextureDepthSettings_None)
	{
		dsvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, arraySize, false);
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = (type == RenderTextureType::RenderTextureType_Tex3D) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = width;
		depthStencilDesc.Height = height;
		depthStencilDesc.DepthOrArraySize = arraySize;
		depthStencilDesc.MipLevels = 1;  
		depthStencilDesc.Format = mResourceFormat;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			nullptr,
			IID_PPV_ARGS(mDepthResource.GetAddressOf())));
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		switch (type)
		{
		case RenderTextureType::RenderTextureType_Tex2D:
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Format = mDepthFormat;
			dsvDesc.Texture2D.MipSlice = 0;
			device->CreateDepthStencilView(mDepthResource.Get(), &dsvDesc, dsvHeap.hCPU(0));
			break;
		default:
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;

			dsvDesc.Texture2DArray.ArraySize = 1;
			dsvDesc.Texture2DArray.MipSlice = 0;
			for (int i = 0; i < arraySize; ++i)
			{
				dsvDesc.Texture2DArray.FirstArraySlice = i;
				device->CreateDepthStencilView(mDepthResource.Get(), &dsvDesc, dsvHeap.hCPU(i));
			}
			break;

		}



	}
}

void RenderTexture::BindColorBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	GetColorViewDesc(srvDesc);
	device->CreateShaderResourceView(mColorResource.Get(), &srvDesc, targetHeap->hCPU(index));
}
void RenderTexture::BindDepthBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	GetDepthViewDesc(srvDesc);
	device->CreateShaderResourceView(mDepthResource.Get(), &srvDesc, targetHeap->hCPU(index));
}
void RenderTexture::BindUAVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT targetMipLevel)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	GetColorUAVDesc(uavDesc, targetMipLevel);
	device->CreateUnorderedAccessView(mColorResource.Get(), nullptr, &uavDesc, targetHeap->hCPU(index));
}