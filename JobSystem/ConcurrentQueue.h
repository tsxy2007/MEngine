#pragma once
#include <mutex>
template <typename T>
class ConcurrentQueue
{
private:
	struct Element
	{
		T value;
	};
	Element* arr;
	int64_t capacity;
	int64_t start = 0;
	int64_t end = 0;
	std::mutex mtx;// Pop must be single thread
public:
	ConcurrentQueue(int64_t capa) : capacity(capa)
	{
		arr = new Element[capacity];
	}
	template <typename Func>
	void Iterate(Func&& func)
	{
		for (int64_t i = start; i < end; ++i)
		{
			func(arr[i % capacity].value);
		}
	}
	void ResizeAndClear(int64_t newCapacity)
	{
		//std::lock_guard<std::mutex> lck(mtx);
		if (newCapacity <= capacity) return;
		int64_t  doubleCapa = capacity * 2;
		newCapacity = newCapacity > doubleCapa ? newCapacity : doubleCapa;
		Element* newArr = (Element*)malloc(sizeof(Element) * newCapacity);
		for (int64_t i = start; i < end; ++i)
		{
			arr[i % capacity].~Element();
		}
		free(arr);
		arr = newArr;
		capacity = newCapacity;
		end = 0;
		start = 0;
	}

	void Push(const T& value)
	{
		std::lock_guard<std::mutex> lck(mtx);
		int64_t currentEnd = end++;
		arr[currentEnd % capacity].value = value;
	}

	bool TryPop(T* value)
	{
		std::lock_guard<std::mutex> lck(mtx);
		int64_t currentStart = start++;
		if (end - currentStart <= 0)
		{
			start--;
			return false;
		}
		*value = arr[currentStart % capacity].value;
		return true;
	}

	~ConcurrentQueue()
	{
		if (arr != nullptr) free(arr);
	}
	int64_t GetSize()
	{
		return end - start;
	}
};