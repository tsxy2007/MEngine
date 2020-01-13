#pragma once
#include "../Common/MObject.h"
#include "../RenderComponent/RenderTexture.h"
#include "../Common/MetaLib.h"
struct ResourceDescriptor
{
	union
	{
		RenderTextureDescriptor rtDesc;
	};
	bool operator!=(const ResourceDescriptor& res) const;
	bool operator==(const ResourceDescriptor& res) const;
};
namespace std
{
	template <>
	class hash<ResourceDescriptor>
	{
	public:
		size_t operator()(const ResourceDescriptor& o) const
		{
			hash<UINT> ulongHash;
			return ulongHash(o.rtDesc.width) ^ 
				ulongHash(o.rtDesc.height) ^ 
				ulongHash(o.rtDesc.depthSlice) ^ 
				ulongHash((UINT)o.rtDesc.type) ^ 
				ulongHash((UINT)o.rtDesc.colorFormat) ^
				ulongHash((UINT)o.rtDesc.depthType);
		}
	};
}
class TempRTAllocator
{
public:
	struct UsingTempRT
	{
		MObject* rt;
		ResourceDescriptor desc;
	};
private:
	struct TempRTData
	{
		MObject* rt;
		UINT containedFrame;
	};
	Dictionary<ResourceDescriptor, std::vector<TempRTData>*> waitingRT;
	std::unordered_map<UINT, UsingTempRT> usingRT;
public:
	TempRTAllocator();
	~TempRTAllocator();
	bool Contains(UINT id);
	MObject* GetUsingRenderTexture(UINT id);
	MObject* GetTempResource(ID3D12Device* device, UINT id, ResourceDescriptor& descriptors);
	void ReleaseRenderTexutre(UINT id);
	void CumulateReleaseAfterFrame();
	//TODO
};

