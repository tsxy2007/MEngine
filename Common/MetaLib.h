#pragma once
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

template <typename F, int count>
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

template <typename F>
struct LoopClass<F, -1>
{
	static void Do(F&& f) {}
};


template <typename F, int count>
void Loop(F&& function)
{
	LoopClass<F, count - 1>::Do(std::move(function));
}
