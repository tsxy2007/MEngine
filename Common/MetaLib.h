#pragma once
typedef unsigned int UINT;
template <typename T, UINT size>
class Storage
{
	alignas(T) char c[size * sizeof(T)];
};
template <typename T>
class Storage<T, 0>
{};


template <typename ... Args>
class AlignedTuple
{
private:
	template <typename ... Args>
	class AlignedTuple_Package;

	template <>
	class AlignedTuple_Package<>
	{
	public:
		static void GetOffset(size_t* offsets, AlignedTuple_Package<>* ptr, unsigned int& index) {}
	};

	template <typename T, typename ... Args>

	class AlignedTuple_Package<T, Args...>
	{
	private:
		std::aligned_storage_t<sizeof(T), alignof(T)> value;
		AlignedTuple_Package<Args...> args;
	public:
		static void GetOffset(size_t* offsets, AlignedTuple_Package<T, Args...>* ptr, unsigned int& index)
		{
			offsets[index] = (size_t)(&(ptr)->value);
			index++;
			AlignedTuple_Package<Args...>::GetOffset(offsets, &ptr->args, index);
		}
	};
	char* arr[sizeof(AlignedTuple_Package<Args...>)];
	size_t offsets[sizeof...(Args)];
public:
	AlignedTuple()
	{
		unsigned int index = 0;
		AlignedTuple_Package<Args...>::GetOffset(offsets, nullptr, index);
	}
	size_t GetOffset(unsigned int index) const
	{
		return offsets[index];
	}
	unsigned int GetObjectCount() const
	{
		return sizeof...(Args);
	}
	void* operator[](unsigned int index)
	{
		return arr + offsets[index];
	}
};

template <typename F, unsigned int count>
struct LoopClass
{
	static void Do(F&& f)
	{
		LoopClass<F, count - 1>::Do(std::move(f));
		f(count);
	}
};

template <typename F>
struct LoopClass<F, 0>
{
	static void Do(F&& f)
	{
		f(0);
	}
};

template <typename F, unsigned int count>
void InnerLoop(F& function)
{
	LoopClass<F, count - 1>::Do(std::move(function));
}

template <typename F, unsigned int count>
void InnerLoop(F&& function)
{
	LoopClass<F, count - 1>::Do(std::move(function));
}
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
	void Reserve(UINT capacity);
	V* operator[](K& key);

	void Add(K& key, V& value);
	void Remove(K& key);

	void Clear();
};
template <typename K, typename V>
void Dictionary<K,V>::Reserve(UINT capacity)
{
	keyDicts.reserve(capacity);
	values.reserve(capacity);
}
template <typename K, typename V>
V* Dictionary<K,V>::operator[](K& key)
{
	auto&& ite = keyDicts.find(key);
	if (ite == keyDicts.end()) return nullptr;
	return &values[ite->second].value;
}
template <typename K, typename V>
void Dictionary<K, V>::Add(K& key, V& value)
{
	keyDicts.insert_or_assign(key, std::move(values.size()));
	values.push_back({ std::move(key), std::move(value) });
}
template <typename K, typename V>
void Dictionary<K, V>::Remove(K& key)
{
	auto&& ite = keyDicts.find(key);
	if (ite == keyDicts.end()) return;
	KVPair& p = values[ite->second];
	p = values[values.size() - 1];
	keyDicts[p.key] = ite->second;
	values.erase(values.end() - 1);
	keyDicts.erase(ite->first);
}
template <typename K, typename V>
void Dictionary<K, V>::Clear()
{
	keyDicts.clear();
	values.clear();
}