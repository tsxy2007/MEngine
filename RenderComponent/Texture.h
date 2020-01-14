#pragma once
#include "../Common/d3dUtil.h"
#include <string>
#include <vector>
#include "../Common/MObject.h"
class FrameResource;
class DescriptorHeap;
enum class TextureType : int
{
	Tex2D = 0,
	Cubemap = 1
};

struct TextureData
{
	UINT width;
	UINT height;
	TextureType textureType;
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

class Texture : public MObject
{
public:

private:
	std::wstring Filename;
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
	TextureType mType;
	DXGI_FORMAT mFormat;
	UINT16 mipLevels;
	void GetResourceViewDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
public:

	std::string Name;
	bool isReadable() const { return UploadHeap == nullptr; }
	bool isAvaliable() const { return Resource == nullptr; }
	ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}
	Texture(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		FrameResource* res,
		std::string name,
		std::wstring filePath,
		bool isDDS,
		TextureType type = TextureType::Tex2D
	);
	void BindColorBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device);
};

