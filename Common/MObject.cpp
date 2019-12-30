#include "MObject.h"
std::atomic<unsigned int> MObject::CurrentID = 0;
std::mutex MObject::mtx;

MObject::MObject()
{
	allPtrs.reserve(20);
	instanceID = CurrentID++;
}

void MObject::AddPtr(PtrLink* ptr)
{
	if (this == nullptr)
	{
		return;
	}
	ptr->value = allPtrs.size();
	allPtrs.push_back(ptr);
}

void MObject::Destroy()
{
	std::lock_guard<std::mutex> lck(mtx);
	if (this == nullptr)
	{
		return;
	}
	for (int i = 0; i < allPtrs.size(); ++i)
	{
		allPtrs[i]->mPtr = nullptr;
	}
	allPtrs.clear();
	delete this;
}

MObject::~MObject()
{
	if (allPtrs.size() <= 0) return;
	std::lock_guard<std::mutex> lck(mtx);
	for (int i = 0; i < allPtrs.size(); ++i)
	{
		allPtrs[i]->mPtr = nullptr;
	}
	allPtrs.clear();
}

void MObject::RemovePtr(PtrLink* ptr)
{
	if (this == nullptr)
	{
		mtx.unlock();
		return;
	}
	allPtrs[ptr->value] = allPtrs[allPtrs.size() - 1];
	memcpy(allPtrs[ptr->value], ptr, sizeof(PtrLink));
	allPtrs.erase(allPtrs.end() - 1);
	bool deleteThis = allPtrs.size() <= 0;
	mtx.unlock();
	if (deleteThis)
	{
		delete this;
	}
}

PtrLink::PtrLink(MObject* ptr) :
	mPtr(ptr)
{
	MObject* obj = (MObject*)(mPtr);
	if (obj != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		obj->AddPtr(this);
	}
}

PtrLink::~PtrLink()
{
	Dispose();
}

PtrLink::PtrLink(const PtrLink& link) :
	mPtr(link.mPtr)
{
	if (mPtr != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		mPtr ->AddPtr(this);
	}
}

PtrLink& PtrLink::operator= (const PtrLink& link)
{
	if (link.mPtr == mPtr) return *this;
	Dispose();
	mPtr = link.mPtr;
	if (mPtr != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		mPtr->AddPtr(this);
	}

	return *this;
}

PtrLink::PtrLink(const PtrLink&& link) :
	mPtr(link.mPtr)
{
	if (mPtr != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		mPtr->AddPtr(this);
	}
}

PtrLink& PtrLink::operator= (const PtrLink&& link)
{
	if (link.mPtr == mPtr) return *this;
	Dispose();
	mPtr = link.mPtr;
	if (mPtr != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		mPtr->AddPtr(this);
	}

	return *this;
}

PtrLink& PtrLink::operator=(MObject* ptr)
{
	if (ptr == mPtr) return *this;
	Dispose();
	mPtr = ptr;
	if (mPtr != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		mPtr->AddPtr(this);
	}
	return *this;
}

void PtrLink::Dispose()
{
	if (mPtr != nullptr)
	{
		MObject::mtx.lock();
		mPtr->RemovePtr(this);
	}
}