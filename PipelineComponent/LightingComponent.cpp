#include "LightingComponent.h"

void LightingComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{

}
void LightingComponent::Dispose()
{

}
std::vector<TemporalResourceCommand>& LightingComponent::SendRenderTextureRequire(EventData& evt)
{
	return tempResources;
}
void LightingComponent::RenderEvent(EventData& data, ThreadCommand* commandList)
{

}