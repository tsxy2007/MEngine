#pragma once
#include "../Common/d3dUtil.h"
class PSOContainer;
class Shader;
enum BackBufferState
{
	BackBufferState_Present = 0,
	BackBufferState_RenderTarget = 1
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
	template <BackBufferState targetState>
	static void TransformBackBufferState(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource
	) {}

	template <>
	static void TransformBackBufferState<BackBufferState_Present>(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource
		)
	{
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				resource,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT
			));
	}
	template <>
	static void TransformBackBufferState<BackBufferState_RenderTarget>(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource
		)
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		));
	}
};