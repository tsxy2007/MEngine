#pragma once
#include <vector>
#include <mutex>
template <typename T>
class Pool
{
private:
	std::vector<T*> allPtrs;
	std::vector<void*> allocatedPtrs;
	int capacity;
	std::mutex mtx;
	void AllocateMemory()
	{
		using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
		Storage* ptr = reinterpret_cast<Storage*>(malloc(sizeof(Storage) * capacity));
		for (int i = 0; i < capacity; ++i)
		{
			allPtrs.push_back(reinterpret_cast<T*>(ptr + i));
		}
		allocatedPtrs.push_back(ptr);
	}
public:
	Pool(int capa) : capacity(capa)
	{
		allPtrs.reserve(capa);
		allocatedPtrs.reserve(10);
		AllocateMemory();
	}
	
	template <typename... Args>
	T* New(Args... args)
	{
		std::lock_guard<std::mutex> lck(mtx);
		if (allPtrs.size() <= 0)
			AllocateMemory();
		T* value = allPtrs[allPtrs.size() - 1];
		allPtrs.erase(allPtrs.end() - 1);
		new (value)T(args...);
		return value;
	}

	void Delete(T* ptr)
	{
		ptr->~T();
		std::lock_guard<std::mutex> lck(mtx);
		allPtrs.push_back(ptr);
	}

	~Pool()
	{
		for (int i = 0; i < allocatedPtrs.size(); ++i)
		{
			free(allocatedPtrs[i]);
		}
	}
};