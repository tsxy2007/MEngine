#pragma once
#include "../Common/d3dUtil.h"
#include <vector>
#include <string>
#include "../Common/MObject.h"
struct Pass
{
	std::string name;
	std::wstring filePath;
	std::string vertex;
	std::string fragment;
	Microsoft::WRL::ComPtr<ID3DBlob> vsShader;
	Microsoft::WRL::ComPtr<ID3DBlob> psShader;
	D3D12_RASTERIZER_DESC rasterizeState;
	D3D12_DEPTH_STENCIL_DESC depthStencilState;
	D3D12_BLEND_DESC blendState;
};

struct ShaderVariable
{
	enum Type
	{
		ConstantBuffer, DescriptorHeap, StructuredBuffer
	};
	std::string name;
	Type type;
	UINT tableSize;
	UINT registerPos;
	UINT space;

};
class Shader
{
private:
	std::vector<Pass> allPasses;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<UINT, UINT> mVariablesDict;
	std::vector<ShaderVariable> mVariablesVector;
public:
	Shader() {}
	~Shader();
	Shader(
		Pass* passes, UINT passCount,
		ShaderVariable* shaderVariables, UINT shaderVarCount,
		ID3D12Device* device,
		bool useShaderCache
	);
	void GetPassPSODesc(UINT pass, D3D12_GRAPHICS_PIPELINE_STATE_DESC* targetPSO);
	ShaderVariable GetVariable(std::string name);
	ShaderVariable GetVariable(UINT id);
	void BindRootSignature(ID3D12GraphicsCommandList* commandList);
	int GetPropertyRootSigPos(UINT id);
	ID3D12RootSignature* GetSignature() const { return mRootSignature.Get(); }
	void SetResource(ID3D12GraphicsCommandList* commandList, UINT id, MObject* targetObj, UINT indexOffset);
	void SetStructuredBufferByAddress(ID3D12GraphicsCommandList* commandList, UINT id, D3D12_GPU_VIRTUAL_ADDRESS address);
	bool TryGetShaderVariable(UINT id, ShaderVariable& targetVar);
	size_t VariableLength() const { return mVariablesVector.size(); }
	template<typename Func>
	void IterateVariables(Func&& f)
	{
		for (int i = 0; i < mVariablesVector.size(); ++i)
		{
			f(mVariablesVector[i]);
		}
	}
};

