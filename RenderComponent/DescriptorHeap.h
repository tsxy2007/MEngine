#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
class DescriptorHeap : public MObject
{
public:
	DescriptorHeap() : MObject(), pDH(nullptr) {}
	HRESULT Create(
		ID3D12Device* pDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		UINT NumDescriptors,
		bool bShaderVisible = false);
	inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& Get() { return pDH; }

	inline D3D12_CPU_DESCRIPTOR_HANDLE hCPU(UINT index)
	{
		if (index >= mNumDescriptors) index = mNumDescriptors - 1;
		D3D12_CPU_DESCRIPTOR_HANDLE h;
		h.ptr = hCPUHeapStart.ptr + index * HandleIncrementSize;
		return h;
	}
	inline D3D12_GPU_DESCRIPTOR_HANDLE hGPU(UINT index)
	{
		if (index >= mNumDescriptors) index = mNumDescriptors - 1;
		D3D12_GPU_DESCRIPTOR_HANDLE h;
		h.ptr = hGPUHeapStart.ptr + index * HandleIncrementSize;
		return h;
	}
	void SetDescriptorHeap(ID3D12GraphicsCommandList* commandList);
	inline D3D12_DESCRIPTOR_HEAP_DESC GetDesc() const { return Desc; };
	virtual ~DescriptorHeap();
private:
	UINT mNumDescriptors = 0;
	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDH;
	D3D12_CPU_DESCRIPTOR_HANDLE hCPUHeapStart;
	D3D12_GPU_DESCRIPTOR_HANDLE hGPUHeapStart;
	UINT HandleIncrementSize;

};