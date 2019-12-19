#pragma once
#include "../RenderComponent/MObject.h"
#include "../RenderComponent/RenderTexture.h"
#define K RenderTextureDescriptor
#define V std::vector<TempRTData>*
class TempRTAllocator
{
private:
	struct TempRTData
	{
		ObjectPtr<RenderTexture> rt;
		UINT containedFrame;
	};

	class Dictionary
	{
	public:
		struct KVPair
		{
			K key;
			V value;
		};
		std::unordered_map<K, UINT> keyDicts;
		std::vector<KVPair> values;
		void Reserve(UINT capacity);
		bool TryGet(K& key, V* value);

		void Add(K& key, V& value);
		void Remove(K& key);

		void Clear();
	};

	Dictionary waitingRT;
public:
	TempRTAllocator();
	~TempRTAllocator();
	void GetRenderTextures(ID3D12Device* device, RenderTextureDescriptor* descriptors, RenderTexture** rtResults, UINT count);
	void CumulateReleaseAfterFrame();
	//TODO
};
#undef K 
#undef V 

