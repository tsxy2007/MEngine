#include "TempRTAllocator.h"

TempRTAllocator::TempRTAllocator()
{
	waitingRT.Reserve(30);
}


TempRTAllocator::~TempRTAllocator()
{
	for (int i = 0; i < waitingRT.values.size(); ++i)
	{
		auto& v = waitingRT.values[i];
		for (int j = 0; j < v.value->size(); ++j)
			(*v.value)[j].rt->Destroy();
		delete v.value;
	}
}

TempRTAllocator::UsingTempRT* TempRTAllocator::GetUsingData(UINT id)
{
	UsingTempRT* ptr = usingRT[id];
	return ptr;
}

RenderTexture* TempRTAllocator::GetRenderTextures(ID3D12Device* device, UINT id, RenderTextureDescriptor& descriptor)
{

	std::vector<TempRTData>* datas = nullptr;
	std::vector<TempRTData>** datasPtr = waitingRT[descriptor];
	if (datasPtr == nullptr)
	{
		datas = new std::vector<TempRTData>();
		waitingRT.Add(descriptor, datas);
	}
	else
		datas = *datasPtr;
	if (datas->size() > 0)
	{
		TempRTData& data = (*datas)[datas->size() - 1];
		UsingTempRT usingRTData;
		usingRTData.rt = data.rt;
		usingRTData.desc = descriptor;
		usingRT.Add(id, usingRTData);
		RenderTexture* result = data.rt.operator->();
		datas->erase(datas->end() - 1);
		return result;
	}
	else
	{
		UsingTempRT usingRTData;
		usingRTData.rt = new RenderTexture(device, descriptor.width, descriptor.height, descriptor.colorFormat, (int)descriptor.depthFormat != 0, descriptor.type, descriptor.depthSlice, 1);;
		usingRTData.desc = descriptor;
		usingRT.Add(id, usingRTData);
		return usingRTData.rt.operator->();
	}
}

void TempRTAllocator::ReleaseRenderTexutre(UINT id)
{
	UsingTempRT* rt = usingRT[id];
	if (rt != nullptr)
	{
		std::vector<TempRTData>** datasPtr = waitingRT[rt->desc];
		if (datasPtr != nullptr)
		{
			(*datasPtr)->push_back({ rt->rt, id });
		}
		usingRT.Remove(id);
	}
}

void TempRTAllocator::CumulateReleaseAfterFrame()
{
	for (UINT i = 0; i < waitingRT.values.size(); ++i)
	{
		std::vector<TempRTData>* data = waitingRT.values[i].value;
		for (UINT j = 0; j < data->size(); ++j)
		{
			TempRTData& d = (*data)[j];
			d.containedFrame++;
			if (d.containedFrame >= 4)
			{
				d.rt->Destroy();
				(*data)[j] = (*data)[data->size() - 1];
				data->erase(data->end() - 1);
				j--;
			}
		}
	}
}