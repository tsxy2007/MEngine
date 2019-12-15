#include "MObject.h"
std::atomic<unsigned int> MObject::CurrentID = 0;


MObject::MObject()
{
	allPtrs.reserve(20);
	instanceID = CurrentID++;
}

MObject::~MObject()
{
	mtx.lock();
	for (int i = 0; i < allPtrs.size(); ++i)
	{
		allPtrs[i]->mPtr = nullptr;
	}
	mtx.unlock();
}

void MObject::AddPtr(PtrLink* ptr)
{
	mtx.lock();
	ptr->value = allPtrs.size();
	allPtrs.push_back(ptr);
	mtx.unlock();
}

void MObject::Destroy()
{
	mtx.lock();
	for (int i = 0; i < allPtrs.size(); ++i)
	{
		allPtrs[i]->mPtr = nullptr;
	}
	allPtrs.clear();
	mtx.unlock();
	delete this;
}

void MObject::RemovePtr(PtrLink* ptr)
{
	mtx.lock();
	allPtrs[ptr->value] = allPtrs[allPtrs.size() - 1];
	memcpy(allPtrs[ptr->value], ptr, sizeof(PtrLink));
	allPtrs.erase(allPtrs.end() - 1);
	bool deleteThis = allPtrs.size() <= 0;
	mtx.unlock();
	if(deleteThis)
		delete this;
	
}

PtrLink::PtrLink(MObject* ptr) :
	mPtr(ptr)
{
	if (mPtr != nullptr)
	{
		mPtr->AddPtr(this);
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
		mPtr->AddPtr(this);
	}
}

PtrLink& PtrLink::operator= (const PtrLink& link)
{
	if (link.mPtr == mPtr) return *this;
	Dispose();
	mPtr = link.mPtr;
	if (mPtr != nullptr)
	{
		mPtr->AddPtr(this);
	}

	return *this;
}

PtrLink::PtrLink(const PtrLink&& link) :
	mPtr(link.mPtr)
{
	if (mPtr != nullptr)
	{
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
		mPtr->AddPtr(this);
	}
	return *this;
}

void PtrLink::Dispose()
{
	if (mPtr != nullptr)
	{
		mPtr->RemovePtr(this);
	}
}