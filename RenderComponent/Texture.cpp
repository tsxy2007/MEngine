#include "Texture.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/DescriptorHeap.h"
#include <fstream>
#include "ComputeShader.h"
#include "../Singleton/ShaderCompiler.h"
#include "RenderCommand.h"
#include "../Singleton/ShaderID.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;
ComputeShader* texCopyShader = nullptr;
bool isInitialized = false;
UINT _RGBA32InputBuffer = 0;
UINT _RGBA64InputBuffer = 0;
UINT _RGBAFloatInputBuffer = 0;
UINT CopyData = 0;
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
	if (headerResult.mipCount < 1) headerResult.mipCount = 1;
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
struct DispatchCBuffer
{
	XMUINT2 _StartPos;
	XMUINT2 _Count;
};
class TextureLoadCommand : public RenderCommand
{
private:
	UploadBuffer ubuffer;
	UploadBuffer dispatchCBuffer;
	DescriptorHeap heap;
	ID3D12Resource* res;
	TextureData::LoadFormat loadFormat;
	UINT width;
	UINT height;
	UINT mip;
public:
	TextureLoadCommand(ID3D12Device* device, UINT element, void* dataPtr, ID3D12Resource* res, TextureData::LoadFormat loadFormat, UINT width, UINT height, UINT mip) :
		res(res), loadFormat(loadFormat), width(width), height(height), mip(mip)
	{
		dispatchCBuffer.Create(device, this->mip, true, sizeof(DispatchCBuffer));
		ubuffer.Create(device, element, false, 1);
		ubuffer.CopyDatas(0, element, dataPtr);
		
		heap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);
		if (!isInitialized)
		{
			texCopyShader = ShaderCompiler::GetComputeShader("TextureCopy");
			_RGBA32InputBuffer = ShaderID::PropertyToID("_RGBA32InputBuffer");
			_RGBA64InputBuffer = ShaderID::PropertyToID("_RGBA64InputBuffer");
			_RGBAFloatInputBuffer = ShaderID::PropertyToID("_RGBAFloatInputBuffer");
			CopyData = ShaderID::PropertyToID("CopyData");
		}
	}
	virtual void operator()(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource)
	{
		ubuffer.ReleaseAfterFlush(resource);
		dispatchCBuffer.ReleaseAfterFlush(resource);
		texCopyShader->BindRootSignature(commandList, nullptr);
		UINT offset = 0;
		UINT curWidth = width;
		UINT curHeight = height;
		for (UINT i = 0; i < mip; ++i)
		{
			DispatchCBuffer bf;
			bf._Count = { curWidth, curHeight };
			bf._StartPos = { 0, 0 };
			dispatchCBuffer.CopyData(i, &bf);
			texCopyShader->SetResource(commandList, CopyData, &dispatchCBuffer, i);

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = res->GetDesc().Format;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = i;
			uavDesc.Texture2D.PlaneSlice = 0;
			device->CreateUnorderedAccessView(res, nullptr, &uavDesc, heap.hCPU(i));
			void* ptrTester = heap.Get().Get();
			texCopyShader->SetResource(commandList, ShaderID::GetMainTex(), &heap, i);
			UINT dispatchKernel = 0;
			switch (loadFormat)
			{
			case TextureData::LoadFormat_RGBA8:
				dispatchKernel = 0;
				texCopyShader->SetStructuredBufferByAddress(commandList, _RGBA32InputBuffer, ubuffer.GetAddress(offset));
				offset += curWidth * curHeight * 4;
				break;
			case TextureData::LoadFormat_RGBA16:
				dispatchKernel = 1;
				texCopyShader->SetStructuredBufferByAddress(commandList, _RGBA64InputBuffer, ubuffer.GetAddress(offset));
				offset += curWidth * curHeight * 8;
				break;
			case TextureData::LoadFormat_RGBAFloat32:
				dispatchKernel = 2;
				texCopyShader->SetStructuredBufferByAddress(commandList, _RGBAFloatInputBuffer, ubuffer.GetAddress(offset));
				offset += curWidth * curHeight * 16;
				break;
			}
			UINT dispX, dispY;
			dispX = (UINT)ceil(curWidth / 8.0);
			dispY = (UINT)ceil(curHeight / 8.0);
			texCopyShader->Dispatch(commandList, dispatchKernel, dispX, dispY, 1);
			curWidth /= 2;
			curHeight /= 2;
		}
	}
};
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
		ZeroMemory(&data, sizeof(TextureData));
		std::vector<char> dataResults;
		ReadData(filePath, data, dataResults);
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		switch (data.format)
		{
		case TextureData::LoadFormat_RGBA8:
			mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case TextureData::LoadFormat_RGBA16:
			mFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
			break;
		case TextureData::LoadFormat_RGBAFloat32:
			mFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			break;
		}
		mipLevels = data.mipCount;
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
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&Resource)));
		
		TextureLoadCommand* cmd = new TextureLoadCommand(
			device,
			dataResults.size() - 1,
			dataResults.data(),
			Resource.Get(),
			data.format,
			data.width,
			data.height,
			data.mipCount);
		RenderCommand::AddCommand(cmd);
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