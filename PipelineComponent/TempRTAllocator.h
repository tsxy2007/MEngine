#pragma once
#include "../RenderComponent/MObject.h"
#include "../RenderComponent/RenderTexture.h"
class TempRTAllocator
{
private:

	template <typename K, typename V>
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
		void Reserve(UINT capacity)
		{
			keyDicts.reserve(capacity);
			values.reserve(capacity);
		}
		bool TryGet(const K& key, V* value)
		{
			auto&& ite = keyDicts.find(key);
			if (ite == keyDicts.end()) return false;
			*value = values[ite->second].value;
			return true;
		}

		void Add(const K& key, const V& value)
		{
			keyDicts[key] = values.size();
			values.push_back({ std::move(key), std::move(value) });
		}

		void Remove(const K& key)
		{
			auto&& ite = keyDicts.find(key);
			if (ite == keyDicts.end()) return;
			KVPair& p = values[ite->second];
			p = values[values.size() - 1];
			keyDicts[p.key] = ite->second;
			values.erase(values.end() - 1);
			keyDicts.erase(ite->first);
		}

		void Clear()
		{
			keyDicts.clear();
			values.clear();
		}
	};
	struct TempRTData
	{
		ObjectPtr<RenderTexture> rt;
		UINT containedFrame;
	};
	Dictionary<RenderTextureDescriptor, TempRTData> usingRT;
	Dictionary<RenderTextureDescriptor, TempRTData> waitingRT;
public:
	TempRTAllocator();
	~TempRTAllocator();
	//TODO
};

