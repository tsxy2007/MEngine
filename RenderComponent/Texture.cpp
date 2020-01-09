#include "Texture.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/DescriptorHeap.h"
#include <fstream>

struct TextureData
{
	UINT width, height;
	UINT mipCount;
	enum LoadFormat
	{
		LoadFormat_RGBA8 = 0,
		LoadFormat_RGBA16 = 1,
		LoadFormat_RGBAFloat32 = 2,
		LoadFormat_Num = 3
	};
	LoadFormat format;
};

void ReadData(std::wstring& str, TextureData& headerResult, std::vector<char>& dataResult)
{
	std::ifstream ifs;
	ifs.open(str, std::ios::binary);
	char header[sizeof(TextureData) + 1];
	ifs.read(header, sizeof(TextureData));
	memcpy(&headerResult, header, sizeof(TextureData));
	UINT formt = (UINT)headerResult.format;
	if (formt >= (UINT)(TextureData::LoadFormat_Num))
	{
		throw "Invalide Format";
	}
	UINT stride = 0;
	switch (headerResult.format)
	{
	case TextureData::LoadFormat_RGBA8:
		stride = 4;
		break;
	case TextureData::LoadFormat_RGBA16:
		stride = 8;
		break;
	case TextureData::LoadFormat_RGBAFloat32:
		stride = 16;
		break;
	}
	size_t size = stride * headerResult.width * headerResult.height;
	UINT width = headerResult.width;
	UINT height = headerResult.height;
	for (UINT i = 0; i < headerResult.mipCount; ++i)
	{
		width /= 2;
		height /= 2;
		size += stride * width * height;
	}
	dataResult.resize(size + 1);
	ifs.read(dataResult.data(), size);
}

Texture::Texture(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	FrameResource* res,
	std::string name,
	std::wstring filePath,
	bool isDDs,
	TextureType type
) : MObject(), mType(type)
{
	Name = name;
	Filename = filePath;
	if (isDDs)
	{
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device,
			commandList, Filename.c_str(),
			Resource, UploadHeap));
		auto desc = Resource->GetDesc();
		mFormat = desc.Format;
		mipLevels = desc.MipLevels;
		FrameResource::ReleaseResourceAfterFlush(UploadHeap, res);
		UploadHeap = nullptr;
	}
	else
	{
		TextureData data;//TODO : Read From Texture
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.Width = data.width;
		texDesc.Height = data.height;
		texDesc.DepthOrArraySize = 1;
		texDesc.MipLevels = data.mipCount;
		texDesc.Format = mFormat;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		D3D12_CLEAR_VALUE colorClear;
		colorClear.Format = mFormat;
		float colors[4] = { 0,0,0,0 };
		memcpy(colorClear.Color, colors, sizeof(float) * 4);
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&colorClear,
			IID_PPV_ARGS(&Resource)));
		std::vector<char> dataResults;
		ReadData(filePath, data, dataResults);
		UploadBuffer upBuffer;
		upBuffer.Create(device, dataResults.size() - 1, false, 1);
		upBuffer.ReleaseAfterFlush(res);
		upBuffer.CopyDatas(0, dataResults.size() - 1, dataResults.data());
	}
}
void Texture::GetResourceViewDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	switch (mType) {
	case Tex2D:
		srvDesc.Format = mFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = mipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case Cubemap:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = mipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		srvDesc.Format = mFormat;
		break;
	}
}

void Texture::BindColorBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	GetResourceViewDescriptor(srvDesc);
	device->CreateShaderResourceView(Resource.Get(), &srvDesc, targetHeap->hCPU(index));
}