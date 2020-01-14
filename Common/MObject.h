#pragma once
#include <atomic>
#include <mutex>
#include <vector>
class PtrLink;
class MObject
{
	friend class PtrLink;
private:
	bool ptrInitialized = false;
	static std::mutex mtx;
	std::vector<PtrLink*> allPtrs;
	void AddPtr(PtrLink* ptr);
	void RemovePtr(PtrLink* ptr, std::unique_lock<std::mutex>&);
	static std::atomic<unsigned int> CurrentID;
	unsigned int instanceID;
public:
	void Destroy();
	unsigned int GetInstanceID() const { return instanceID; }
	MObject();
	virtual ~MObject();
};

class PtrLink final
{
private:
	void Dispose()  noexcept;
public:
	MObject* mPtr;
	int value;
	PtrLink(MObject* ptr) noexcept;

	PtrLink& operator= (const PtrLink& link) noexcept;

	PtrLink(const PtrLink& link) noexcept;

	~PtrLink() noexcept;
};


template <typename T>
class ObjectPtr
{
private:
	PtrLink link;

public:
	ObjectPtr(T* ptr) noexcept :
		link(ptr)
	{}

	ObjectPtr() noexcept :
		link(nullptr) {}
	ObjectPtr(const ObjectPtr<T>& ptr) noexcept :
		link(ptr.link)
	{

	}

	operator bool() const noexcept
	{
		return link.mPtr != nullptr;
	}

	operator MObject*() const noexcept
	{
		return link.mPtr;
	}

	ObjectPtr(const PtrLink& ptr) noexcept : link(ptr)
	{

	}
	template<typename F>
	ObjectPtr<F> Cast() const noexcept
	{
		ObjectPtr<F> mobj = link;
		return mobj;
	}

	ObjectPtr<T>& operator=(const ObjectPtr<T>& other) noexcept
	{
		link = other.link;
		return *this;
	}

	T* operator->() noexcept
	{
		return (T*)link.mPtr;
	}

	T& operator*() noexcept
	{
		return *(T*)link.mPtr;
	}

	bool operator==(const T* ptr) const noexcept
	{
		return link.mPtr == ptr;
	}
	bool operator!=(const T* ptr) const noexcept
	{
		return link.mPtr != ptr;
	}
	bool operator==(const ObjectPtr<T>& ptr) const noexcept
	{
		return link.mPtr == ptr.link.mPtr;
	}
	bool operator!=(const ObjectPtr<T>& ptr) const noexcept
	{
		return link.mPtr != ptr.link.mPtr;
	}
};