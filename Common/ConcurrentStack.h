#pragma once
#include <atomic>
#include <mutex>
typedef long long int64;
template<typename T>
class ConcurrentStack
{
private:
	T* datas;
	std::atomic<int64> length;
	size_t mCapacity;
	std::mutex mtx;
public:
	ConcurrentStack(size_t capacity) :
		mCapacity(capacity),
		length(0)
	{
		if (mCapacity < 3) mCapacity = 3;
		size_t sz = sizeof(T) * mCapacity;
		datas = (T*)malloc(sz);
	}
	unsigned int Push(const T& value)
	{
		int64 index = -1;
		do
		{
			index = length++;
			if (index >= (int)mCapacity)
			{
				mtx.lock();
				if (index >= mCapacity)
				{
					unsigned int newCapacity = mCapacity * 2;
					T* newDatas = (T*)malloc(sizeof(T) * newCapacity);
					memcpy(newDatas, datas, sizeof(T) * mCapacity);
					free(datas);
					datas = newDatas;
					mCapacity = newCapacity;
				}
				mtx.unlock();
			}
		} while (index < 0);
		datas[index] = value;
		return (unsigned int)index;
	}
	void Set(unsigned int index, const T& value)
	{
		mtx.lock();
		datas[index] = value;
		mtx.unlock();
	}

	T Get(unsigned int index)
	{
		mtx.lock();
		T value = datas[index];
		mtx.unlock();
		return value;
	}
	bool TryPop(T* data)
	{
		int64 index = --length;
		if (index < 0) return false;
		if (index >= (int)mCapacity)
		{
			mtx.lock();
			mtx.unlock();
		}
		*data = datas[index];
		return true;
	}
	int64 Pop(T* data)
	{
		int64 index = --length;
		if (index < 0) return -1;
		if (index >= (int)mCapacity)
		{
			mtx.lock();
			mtx.unlock();
		}
		*data = datas[index];
		return index;
	}
	int64 size() const
	{
		return length;
	}
	~ConcurrentStack()
	{
		if (datas != nullptr)
		{
			free(datas);
		}
	}
};