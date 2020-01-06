#pragma once
#include "../Common/d3dUtil.h"
#include <string>
#include <vector>
#include "../Common/MObject.h"
#include "../RenderComponent/DescriptorHeap.h"
class Texture : public MObject
{
public:
	enum TextureType
	{
		Tex2D, Tex3D, Cubemap
	};
private:
	std::wstring Filename;
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
	TextureType mType;
	void GetResourceViewDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
public:

	std::string Name;
	bool isReadable() const { return UploadHeap == nullptr; }
	bool isAvaliable() const { return Resource == nullptr; }
	ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}
	
	void MakeNoLongerReadable()
	{
		UploadHeap = nullptr;
	}
	Texture(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		std::string name,
		std::wstring filePath,
		TextureType type = Tex2D
	);
	void BindColorBufferToSRVHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device);
};

