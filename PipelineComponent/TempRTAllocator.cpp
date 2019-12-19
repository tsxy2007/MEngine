#include "TempRTAllocator.h"
#define K RenderTextureDescriptor
#define V std::vector<TempRTData>*
void TempRTAllocator::Dictionary::Reserve(UINT capacity)
{
	keyDicts.reserve(capacity);
	values.reserve(capacity);
}
bool TempRTAllocator::Dictionary::TryGet(K& key, V* value)
{
	auto&& ite = keyDicts.find(key);
	if (ite == keyDicts.end()) return false;
	*value = values[ite->second].value;
	return true;
}

void TempRTAllocator::Dictionary::Add(K& key, V& value)
{
	keyDicts[key] = values.size();
	values.push_back({ std::move(key), std::move(value) });
}

void TempRTAllocator::Dictionary::Remove(K& key)
{
	auto&& ite = keyDicts.find(key);
	if (ite == keyDicts.end()) return;
	KVPair& p = values[ite->second];
	p = values[values.size() - 1];
	keyDicts[p.key] = ite->second;
	values.erase(values.end() - 1);
	keyDicts.erase(ite->first);
}

void TempRTAllocator::Dictionary::Clear()
{
	keyDicts.clear();
	values.clear();
}
#undef K 
#undef V 

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

void TempRTAllocator::GetRenderTextures(ID3D12Device* device, RenderTextureDescriptor* descriptors, RenderTexture** rtResults, UINT count)
{
	std::vector<ObjectPtr<RenderTexture>> cacheResults(count);
	for (UINT i = 0; i < count; ++i)
	{
		std::vector<TempRTData>* datas = nullptr;
		if (!waitingRT.TryGet(descriptors[i], &datas))
		{
			datas = new std::vector<TempRTData>();
			waitingRT.Add(descriptors[i], datas);
		}
		if (datas->size() > 0)
		{
			TempRTData& data = (*datas)[datas->size() - 1];
			cacheResults[i] = data.rt;
			datas->erase(datas->end() - 1);
		}
		else
		{
			auto& desc = descriptors[i];
			cacheResults[i] = new RenderTexture(device, desc.width, desc.height, desc.colorFormat, (int)desc.depthFormat != 0, desc.type, desc.depthSlice, 1);
		}
	}
	for (UINT i = 0; i < count; ++i)
	{
		rtResults[i] = cacheResults[i].operator->();
		std::vector<TempRTData>* datas = nullptr;
		if (!waitingRT.TryGet(descriptors[i], &datas))
		{
			datas = new std::vector<TempRTData>();
			waitingRT.Add(descriptors[i], datas);
		}
		datas->push_back({ cacheResults[i], 0 });
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