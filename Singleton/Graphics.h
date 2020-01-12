#pragma once
#include "../Common/d3dUtil.h"
class PSOContainer;
class Shader;
class RenderTexture;
enum BackBufferState
{
	BackBufferState_Present = 0,
	BackBufferState_RenderTarget = 1
};

enum CopyTarget
{
	CopyTarget_DepthBuffer = 0,
	CopyTarget_ColorBuffer = 1
};

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
	template <D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState>
	static void ResourceStateTransform(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource) 
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			beforeState,
			afterState
		));
	}

	static void CopyTexture(
		ID3D12GraphicsCommandList* commandList,
		RenderTexture* source, CopyTarget sourceTarget,
		RenderTexture* dest, CopyTarget destTarget);
};