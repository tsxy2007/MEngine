#include "Shader.h"
#include "../Singleton/ShaderID.h"
#include "Texture.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "UploadBuffer.h"
#include <fstream>
using namespace std;
using Microsoft::WRL::ComPtr;
Shader::~Shader()
{

}

void Shader::BindRootSignature(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetGraphicsRootSignature(mRootSignature.Get());
}

void Shader::GetPassPSODesc(UINT pass, D3D12_GRAPHICS_PIPELINE_STATE_DESC* targetPSO)
{
	Pass& p = allPasses[pass];
	targetPSO->VS =
	{
		reinterpret_cast<BYTE*>(p.vsShader->GetBufferPointer()),
		p.vsShader->GetBufferSize()
	};
	targetPSO->PS =
	{
		reinterpret_cast<BYTE*>(p.psShader->GetBufferPointer()),
		p.psShader->GetBufferSize()
	};
	targetPSO->BlendState = p.blendState;
	targetPSO->RasterizerState = p.rasterizeState;
	targetPSO->pRootSignature = mRootSignature.Get();
	targetPSO->DepthStencilState = p.depthStencilState;
}

ComPtr<ID3DBlob> CompileShader(std::wstring& path, std::string& functionName, std::string&& standard, D3D_SHADER_MACRO* macros, bool useCache)
{
	//std::fstream fs;
	std::wstring filePath = path + L".hlsl";
	/*std::wstring wstr(functionName.size() + 1, L' ');
	wstr[0] = L'_';
	for (int i = 0; i < functionName.size(); ++i)
	{
		wstr[i + 1] = functionName[i];
	}
	std::wstring cachePath = path + wstr + L".shaderCache";
	fs.open(cachePath, ios::in);*/
	ComPtr<ID3DBlob> result;
	if(!useCache /*|| !fs*/)
	{ 
	//	fs.close();
	//	fs.open(cachePath, ios::out);
		result = d3dUtil::CompileShader(filePath, macros, functionName, standard);
	//	fs.write((char*)result->GetBufferPointer(), result->GetBufferSize());
	//	fs.close();
	}
	else
	{
	/*	fs.seekg(0, fs.end);
		size_t shaderSize = fs.tellg();
		fs.seekg(0, 0);
		ThrowIfFailed(D3DCreateBlob(shaderSize, result.GetAddressOf()));
		fs.read((char*)result->GetBufferPointer(), shaderSize);
		size_t flag = fs.tellg();
		fs.close();*/
	}
	return result;
}

Shader::Shader(
	std::vector<Pass> passPaths,
	std::vector<ShaderVariable> allShaderVariables,
	ID3D12Device* device,
	bool useShaderCache
)
{
	//Create Pass
	allPasses.reserve(passPaths.size());
	for (int i = 0; i < passPaths.size(); ++i)
	{
		Pass& p = passPaths[i];
		if (p.vsShader == nullptr)
			p.vsShader = CompileShader(p.filePath, p.vertex, "vs_5_1", nullptr, useShaderCache);// d3dUtil::CompileShader(p.filePath, nullptr, p.vertex, "vs_5_1");
		if (p.psShader == nullptr)
			p.psShader = CompileShader(p.filePath, p.fragment, "ps_5_1", nullptr, useShaderCache);// d3dUtil::CompileShader(p.filePath, nullptr, p.fragment, "ps_5_1");
		allPasses.push_back(std::move(p));
	}
	mVariablesDict.reserve(allShaderVariables.size() + 2);
	mVariablesVector.reserve(allShaderVariables.size());
	for (int i = 0; i < allShaderVariables.size(); ++i)
	{
		ShaderVariable& variable = allShaderVariables[i];
		mVariablesDict[ShaderID::PropertyToID(variable.name)] = i;
		mVariablesVector.push_back(variable);

	}

	vector<CD3DX12_ROOT_PARAMETER> allParameter;
	auto staticSamplers = d3dUtil::GetStaticSamplers();
	allParameter.reserve(VariableLength());
	std::vector< CD3DX12_DESCRIPTOR_RANGE> allTexTable;
	IterateVariables([&](ShaderVariable& var) -> void {
		if (var.type == ShaderVariable::Type::DescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
	});
	UINT offset = 0;
	IterateVariables([&](ShaderVariable& var) -> void
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter;
		switch (var.type)
		{
		case ShaderVariable::Type::DescriptorHeap:
			slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
			offset++;
			break;
		case ShaderVariable::Type::ConstantBuffer:
			slotRootParameter.InitAsConstantBufferView(var.registerPos, var.space);
			break;
		case ShaderVariable::Type::StructuredBuffer:
			slotRootParameter.InitAsShaderResourceView(var.registerPos, var.space);
			break;
		default:
			return;
		}
		allParameter.push_back(slotRootParameter);
	});
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(allParameter.size(), allParameter.data(),
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

int Shader::GetPropertyRootSigPos(UINT id)
{
	auto&& ite = mVariablesDict.find(id);
	if (ite == mVariablesDict.end()) return -1;
	return (int)ite->second;
}

void Shader::SetResource(ID3D12GraphicsCommandList* commandList, UINT id, MObject* targetObj, UINT indexOffset)
{
	if (targetObj == nullptr) return;
	auto&& ite = mVariablesDict.find(id);
	if (ite == mVariablesDict.end()) return;
	UINT rootSigPos = ite->second;
	ShaderVariable& var = mVariablesVector[rootSigPos];
	UploadBuffer* uploadBufferPtr;
	switch (var.type)
	{
	case ShaderVariable::Type::DescriptorHeap:
		commandList->SetGraphicsRootDescriptorTable(
			rootSigPos,
			((DescriptorHeap*)targetObj)->hGPU(indexOffset)
		);
		break;
	case ShaderVariable::Type::ConstantBuffer:
		uploadBufferPtr = ((UploadBuffer*)targetObj);
		commandList->SetGraphicsRootConstantBufferView(
			rootSigPos,
			uploadBufferPtr->Resource()->GetGPUVirtualAddress() + indexOffset * uploadBufferPtr->GetAlignedStride()
		);
		break;
	case ShaderVariable::Type::StructuredBuffer:
		uploadBufferPtr = ((UploadBuffer*)targetObj);
		commandList->SetGraphicsRootShaderResourceView(
			rootSigPos,
			uploadBufferPtr->Resource()->GetGPUVirtualAddress() + indexOffset * uploadBufferPtr->GetStride());
		break;
	}
}

ShaderVariable Shader::GetVariable(std::string name)
{
	return mVariablesVector[mVariablesDict[ShaderID::PropertyToID(name)]];
}

ShaderVariable Shader::GetVariable(UINT id)
{
	return mVariablesVector[mVariablesDict[id]];
}

bool Shader::TryGetShaderVariable(UINT id, ShaderVariable& targetVar)
{
	auto&& ite = mVariablesDict.find(id);
	if (ite == mVariablesDict.end()) return false;
	targetVar = mVariablesVector[ite->second];
	return true;
}

void Shader::SetStructuredBufferByAddress(ID3D12GraphicsCommandList* commandList, UINT id, D3D12_GPU_VIRTUAL_ADDRESS address)
{
	auto&& ite = mVariablesDict.find(id);
	if (ite == mVariablesDict.end()) return;
	UINT rootSigPos = ite->second;
	commandList->SetGraphicsRootShaderResourceView(
		rootSigPos,
		address);
}
