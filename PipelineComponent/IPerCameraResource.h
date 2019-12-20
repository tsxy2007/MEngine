#pragma once
#include <unordered_map>
class Camera;
class IPerCameraResource;
class PipelineComponent;
class FrameResource;
struct PerCameraData final
{
	std::unordered_map<size_t, IPerCameraResource*> resources;
	~PerCameraData();
};
class IPerCameraResource
{
public:
	virtual ~IPerCameraResource() {}
};