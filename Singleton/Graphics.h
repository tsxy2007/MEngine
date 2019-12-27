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
		D3D12_CPU_DESCRIPTOR_HANDLE renderTarget,
		PSOContainer* container,
		Shader* shader, UINT pass);
};