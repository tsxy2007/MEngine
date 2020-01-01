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
		usingRT.insert_or_assign(id, usingRTData);
		RenderTexture* result = data.rt.operator->();
		datas->erase(datas->end() - 1);
		return result;
	}
	else
	{
		UsingTempRT usingRTData;
		usingRTData.rt = new RenderTexture(device, descriptor.width, descriptor.height, descriptor.colorFormat, descriptor.depthType, descriptor.type, descriptor.depthSlice, 1);;
		usingRTData.desc = descriptor;
		usingRT.insert_or_assign(id, usingRTData);
		return usingRTData.rt.operator->();
	}
}
bool TempRTAllocator::Contains(UINT id)
{
	return usingRT.find(id) != usingRT.end();
}

void TempRTAllocator::ReleaseRenderTexutre(UINT id)
{
	auto&& ite = usingRT.find(id);
	if (ite != usingRT.end())
	{
		std::vector<TempRTData>** datasPtr = waitingRT[ite->second.desc];
		
		if (datasPtr != nullptr)
		{
			std::vector<TempRTData>* data = *datasPtr;
			TempRTData rtData;
			rtData.rt = ite->second.rt;
			rtData.containedFrame = 0;
			data->emplace_back(rtData);
			int size = data->size();
			size = data->size();
		}
		usingRT.erase(id);
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