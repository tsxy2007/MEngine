#include "MObject.h"
std::atomic<unsigned int> MObject::CurrentID = 0;
std::mutex MObject::mtx;

MObject::MObject()
{
	instanceID = CurrentID++;
}

void MObject::AddPtr(PtrLink* ptr)
{
	if (this == nullptr)
	{
		return;
	}
	if (!ptrInitialized)
	{
		allPtrs.reserve(10);
		ptrInitialized = true;
	}
	ptr->value = allPtrs.size();
	allPtrs.push_back(ptr);
}

void MObject::Destroy()
{
	{
		std::lock_guard<std::mutex> lck(mtx);
		if (this == nullptr)
		{
			return;
		}
		for (auto ite = allPtrs.begin(); ite != allPtrs.end(); ++ite)
		{
			(*ite)->mPtr = nullptr;
		}
		allPtrs.clear();
	}
	delete this;
}

MObject::~MObject()
{
	if (allPtrs.size() <= 0) return;
	for (auto ite = allPtrs.begin(); ite != allPtrs.end(); ++ite)
	{
		(*ite)->mPtr = nullptr;
	}
	allPtrs.clear();
}

void MObject::RemovePtr(PtrLink* ptr, std::unique_lock<std::mutex>& lck)
{
	if (this == nullptr)
	{
		return;
	}
	auto&& ite = allPtrs.end() - 1;
	allPtrs[ptr->value] = *ite;
	memcpy(allPtrs[ptr->value], ptr, sizeof(PtrLink));
	allPtrs.erase(ite);
	lck.unlock();
	bool deleteThis = allPtrs.empty();
	if (deleteThis)
	{
		delete this;
	}
}

PtrLink::PtrLink(MObject* ptr)  noexcept :
	mPtr(ptr)
{
	MObject* obj = (MObject*)(mPtr);
	if (obj != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		obj->AddPtr(this);
	}
}

PtrLink::~PtrLink() noexcept
{
	Dispose();
}

PtrLink::PtrLink(const PtrLink& link)  noexcept :
	mPtr(link.mPtr)
{
	if (mPtr != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		mPtr->AddPtr(this);
	}
}

PtrLink& PtrLink::operator= (const PtrLink& link)  noexcept
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

void PtrLink::Dispose() noexcept
{
	if (mPtr != nullptr)
	{
		std::unique_lock<std::mutex> lck(MObject::mtx);
		mPtr->RemovePtr(this, lck);
	}
}