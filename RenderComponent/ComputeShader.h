#pragma once
#include "../Common/d3dUtil.h"
#include "Shader.h"

struct ComputeShaderVariable
{
	enum Type
	{
		ConstantBuffer, StructuredBuffer, RWStructuredBuffer, SRVDescriptorHeap, UAVDescriptorHeap
	};
	std::string name;
	Type type;
	UINT tableSize;
	UINT registerPos;
	UINT space;

};

class DescriptorHeap;
class ComputeShader
{
private:
	std::vector<Pass> allPasses;
	
	std::unordered_map<UINT, UINT> mVariablesDict;
	std::vector<ComputeShaderVariable> mVariablesVector;
	std::vector<Microsoft::WRL::ComPtr<ID3DBlob>> csShaders;
	std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pso;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
public:
	ComputeShader(
		std::wstring compilePath,
		std::vector<std::string>& kernelName,
		std::vector<ComputeShaderVariable>& allShaderVariables,
		ID3D12Device* device,
		bool useCache);
	size_t VariableLength() const { return mVariablesVector.size(); }
	void BindRootSignature(ID3D12GraphicsCommandList* commandList, DescriptorHeap* heap);
	void SetResource(ID3D12GraphicsCommandList* commandList, UINT id, MObject* targetObj, UINT indexOffset);
	void SetStructuredBufferByAddress(ID3D12GraphicsCommandList* commandList, UINT id, D3D12_GPU_VIRTUAL_ADDRESS address);
	void Dispatch(ID3D12GraphicsCommandList* cList, UINT kernel, UINT x, UINT y, UINT z);
	template<typename Func>
	void IterateVariables(Func&& f)
	{
		for (int i = 0; i < mVariablesVector.size(); ++i)
		{
			f(mVariablesVector[i]);
		}
	}
};