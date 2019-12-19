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
	mtx.lock();
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
	mtx.unlock();
}

MObject::~MObject()
{
	if (allPtrs.size() <= 0) return;
	mtx.lock();
	for (int i = 0; i < allPtrs.size(); ++i)
	{
		allPtrs[i]->mPtr = nullptr;
	}
	allPtrs.clear();
	mtx.unlock();
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
		MObject::mtx.lock();
		obj->AddPtr(this);
		MObject::mtx.unlock();
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
		MObject::mtx.lock();
		mPtr ->AddPtr(this);
		MObject::mtx.unlock();
	}
}

PtrLink& PtrLink::operator= (const PtrLink& link)
{
	if (link.mPtr == mPtr) return *this;
	Dispose();
	mPtr = link.mPtr;
	if (mPtr != nullptr)
	{
		MObject::mtx.lock();
		mPtr->AddPtr(this);
		MObject::mtx.unlock();
	}

	return *this;
}

PtrLink::PtrLink(const PtrLink&& link) :
	mPtr(link.mPtr)
{
	if (mPtr != nullptr)
	{
		MObject::mtx.lock();
		mPtr->AddPtr(this);
		MObject::mtx.unlock();
	}
}

PtrLink& PtrLink::operator= (const PtrLink&& link)
{
	if (link.mPtr == mPtr) return *this;
	Dispose();
	mPtr = link.mPtr;
	if (mPtr != nullptr)
	{
		MObject::mtx.lock();
		mPtr->AddPtr(this);
		MObject::mtx.unlock();
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
		MObject::mtx.lock();
		mPtr->AddPtr(this);
		MObject::mtx.unlock();
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