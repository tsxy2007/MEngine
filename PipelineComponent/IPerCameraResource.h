#pragma once
#include <unordered_map>
class Camera;
class PipelineComponent;
class FrameResource;
class IPipelineResource
{
public:
	virtual ~IPipelineResource() {}
};