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
	void Dispose();
public:
	MObject* mPtr;
	int value;
	PtrLink(MObject* ptr);

	PtrLink& operator= (const PtrLink& link);

	PtrLink(const PtrLink& link);

	~PtrLink();
};


template <typename T>
class ObjectPtr
{
private:
	PtrLink link;

public:
	ObjectPtr(T* ptr) :
		link(ptr)
	{}

	ObjectPtr() :
		link(nullptr) {}
	ObjectPtr(const ObjectPtr<T>& ptr) :
		link(ptr.link)
	{

	}

	operator bool() const
	{
		return link.mPtr != nullptr;
	}

	operator MObject*() const
	{
		return link.mPtr;
	}

	ObjectPtr(const PtrLink& ptr) : link(ptr)
	{

	}
	template<typename F>
	ObjectPtr<F> Cast() const
	{
		ObjectPtr<F> mobj = link;
		return mobj;
	}

	ObjectPtr<T>& operator=(const ObjectPtr<T>& other)
	{
		link = other.link;
		return *this;
	}

	T* operator->()
	{
		return (T*)link.mPtr;
	}

	T& operator*()
	{
		return *(T*)link.mPtr;
	}

	bool operator==(const T* ptr) const
	{
		return link.mPtr == ptr;
	}
	bool operator!=(const T* ptr) const
	{
		return link.mPtr != ptr;
	}
	bool operator==(const ObjectPtr<T>& ptr) const
	{
		return link.mPtr == ptr.link.mPtr;
	}
	bool operator!=(const ObjectPtr<T>& ptr) const
	{
		return link.mPtr != ptr.link.mPtr;
	}
};