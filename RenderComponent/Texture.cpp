#include "Texture.h"

Texture::Texture(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	std::string name,
	std::wstring filePath,
	TextureType type
) : MObject(), mType(type)
{
	Name = name;
	Filename = filePath;
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device,
		commandList, Filename.c_str(),
		Resource, UploadHeap));
}
void Texture::GetResourceViewDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	switch (mType) {
	case Tex2D:
		srvDesc.Format = Resource->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = Resource->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case Cubemap:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = Resource->GetDesc().MipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		srvDesc.Format = Resource->GetDesc().Format;
		break;
	}
}

void Texture::BindColorBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	GetResourceViewDescriptor(srvDesc);
	device->CreateShaderResourceView(Resource.Get(), &srvDesc, targetHeap->hCPU(index));
}