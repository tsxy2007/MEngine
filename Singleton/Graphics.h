#pragma once
#include "../Common/d3dUtil.h"
class PSOContainer;
class Shader;
class Graphics
{
public:
	static void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	static void Blit(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		D3D12_CPU_DESCRIPTOR_HANDLE* renderTarget,
		UINT renderTargetCount,
		D3D12_CPU_DESCRIPTOR_HANDLE* depthTarget,
		PSOContainer* container,
		UINT width, UINT height,
		Shader* shader, UINT pass);
};