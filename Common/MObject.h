#pragma once
#include <atomic>
#include <mutex>
#include <vector>
class PtrLink;
class MObject
{
	friend class PtrLink;
private:
	static std::mutex mtx;
	std::vector<PtrLink*> allPtrs;
	void AddPtr(PtrLink* ptr);
	void RemovePtr(PtrLink* ptr);
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

	PtrLink& operator=(const PtrLink&& link);

	PtrLink& operator= (const PtrLink& link);

	PtrLink(const PtrLink& link);

	PtrLink(const PtrLink&& link);

	~PtrLink();

	PtrLink& operator=(MObject* ptr);
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

	ObjectPtr(const ObjectPtr<T>&& ptr) :
		link(std::move(ptr.link))
	{

	}

	ObjectPtr(const PtrLink& ptr) : link(ptr)
	{

	}
	template<typename F>
	ObjectPtr<F> Cast()
	{
		ObjectPtr<F> mobj = link;
		return mobj;
	}

	ObjectPtr<T>& operator=(const ObjectPtr<T>& other)
	{
		link = other.link;
		return *this;
	}

	ObjectPtr<T>& operator=(const ObjectPtr<T>&& other)
	{
		link = other.link;
		return *this;
	}

	ObjectPtr<T> & operator=(T* ptr)
	{
		link = ptr;
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