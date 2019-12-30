#pragma once
#include "../Common/MObject.h"
#include "../RenderComponent/RenderTexture.h"
#include "../Common/MetaLib.h"
class TempRTAllocator
{
public:
	struct UsingTempRT
	{
		ObjectPtr<RenderTexture> rt;
		RenderTextureDescriptor desc;
	};
private:
	struct TempRTData
	{
		ObjectPtr<RenderTexture> rt;
		UINT containedFrame;
	};
	Dictionary<RenderTextureDescriptor, std::vector<TempRTData>*> waitingRT;
	std::unordered_map<UINT, UsingTempRT> usingRT;
public:
	TempRTAllocator();
	~TempRTAllocator();
	bool Contains(UINT id);
	RenderTexture* GetRenderTextures(ID3D12Device* device, UINT id, RenderTextureDescriptor& descriptors);
	void ReleaseRenderTexutre(UINT id);
	void CumulateReleaseAfterFrame();
	//TODO
};

