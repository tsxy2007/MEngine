#include "ComputeShader.h"
#include "../Singleton/ShaderID.h"
#include "UploadBuffer.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../Common/d3dUtil.h"
#include <fstream>
#include "StructuredBuffer.h"

using Microsoft::WRL::ComPtr;
ComPtr<ID3DBlob> Compile(std::wstring& path, std::string& kernelName, D3D_SHADER_MACRO* m, bool useCache)
{
	//std::fstream fs;
	std::wstring filePath = path + L".compute";
/*	std::wstring kernelLFilePath(kernelName.size() + 1, L'_');
	kernelLFilePath[0] = L'_';
	for (int i = 0; i < kernelName.size(); ++i)
	{
		kernelLFilePath[1 + i] = kernelName[i];
	}
	std::wstring cachePath = path + kernelLFilePath + L".computeCache";*/
	//fs.open(cachePath, std::ios::in | std::ios::binary);
	ComPtr<ID3DBlob> blob = nullptr;
	//useCache = false;
	if (!useCache/* || !fs*/)
	{
		//fs.close();
	//	fs.open(cachePath, std::ios::out | std::ios::binary);
	//	fs.clear();
	//	fs.seekg(0, std::ios::beg);
		blob = d3dUtil::CompileShader(filePath, m, kernelName, "cs_5_1");
		//fs.write((char*)blob->GetBufferPointer(), blob->GetBufferSize());
	//	fs.close();
	}
	else
	{
		/*
		fs.seekg(0, fs.end);
		size_t sz = fs.tellg();
		fs.clear();
		fs.seekg(0, std::ios::beg);
		ThrowIfFailed(D3DCreateBlob(sz, blob.GetAddressOf()));
		fs.read((char*)blob->GetBufferPointer(), sz);
		ComPtr<ID3DBlob> testOne = d3dUtil::CompileShader(filePath, m, kernelName, "cs_5_1");
		size_t value = false;
		
		for (size_t i = 0; i < testOne->GetBufferSize(); ++i)
		{
			char* c = (char*)blob->GetBufferPointer() + i;
			char* cc = (char*)testOne->GetBufferPointer() + i;
			if (*c != *cc) 
				value = true;
		}*/
	}
	return blob;
}
using namespace std;
ComputeShader::ComputeShader(
	wstring compilePath,
	std::string* kernelName, UINT kernelCount,
	ComputeShaderVariable* allShaderVariables, UINT varSize,
	ID3D12Device* device,
	bool useCache) : csShaders(kernelCount), pso(kernelCount)
{
	mVariablesDict.reserve(varSize + 2);
	mVariablesVector.reserve(varSize);
	for (int i = 0; i < varSize; ++i)
	{
		ComputeShaderVariable& variable = allShaderVariables[i];
		mVariablesDict[ShaderID::PropertyToID(variable.name)] = i;
		mVariablesVector.push_back(variable);

	}

	vector<CD3DX12_ROOT_PARAMETER> allParameter;
	auto staticSamplers = d3dUtil::GetStaticSamplers();
	allParameter.reserve(VariableLength());
	std::vector< CD3DX12_DESCRIPTOR_RANGE> allTexTable;
	IterateVariables([&](ComputeShaderVariable& var) -> void {
		if (var.type == ComputeShaderVariable::Type::SRVDescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
		else if (var.type == ComputeShaderVariable::Type::UAVDescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
	});
	UINT offset = 0;
	IterateVariables([&](ComputeShaderVariable& var) -> void
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter;
		switch (var.type)
		{
		case ComputeShaderVariable::Type::SRVDescriptorHeap:
			slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
			offset++;
			break;
		case ComputeShaderVariable::Type::UAVDescriptorHeap:
			slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
			offset++;
			break;
		case ComputeShaderVariable::Type::ConstantBuffer:
			slotRootParameter.InitAsConstantBufferView(var.registerPos, var.space);
			break;
		case ComputeShaderVariable::Type::StructuredBuffer:
			slotRootParameter.InitAsShaderResourceView(var.registerPos, var.space);
			break;
		case ComputeShaderVariable::Type::RWStructuredBuffer:
			slotRootParameter.InitAsUnorderedAccessView(var.registerPos, var.space);
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
	UINT kernelSize = kernelCount;
	for (int i = 0; i < kernelSize; ++i)
	{
		csShaders[i] = d3dUtil::CompileShader(compilePath, nullptr, kernelName[i], "cs_5_1");//Compile(compilePath, kernelName[i], nullptr, useCache);
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.CS =
		{
			reinterpret_cast<BYTE*>(csShaders[i]->GetBufferPointer()),
			csShaders[i]->GetBufferSize()
		};
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso[i])));
	}

	D3D12_COMMAND_SIGNATURE_DESC desc = {};
	D3D12_INDIRECT_ARGUMENT_DESC indDesc;
	ZeroMemory(&indDesc, sizeof(D3D12_INDIRECT_ARGUMENT_DESC));
	indDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

	desc.ByteStride = sizeof(UINT) * 3;
	desc.NodeMask = 0;
	desc.NumArgumentDescs = 1;
	desc.pArgumentDescs = &indDesc;
	ThrowIfFailed(device->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&mCommandSignature)));
}



void ComputeShader::BindRootSignature(ID3D12GraphicsCommandList* commandList, DescriptorHeap* heap)
{
	commandList->SetComputeRootSignature(mRootSignature.Get());
	if(heap != nullptr)
		commandList->SetDescriptorHeaps(1, heap->Get().GetAddressOf());
}

void ComputeShader::SetResource(ID3D12GraphicsCommandList* commandList, UINT id, MObject* targetObj, UINT indexOffset)
{
	if (targetObj == nullptr) return;
	auto&& ite = mVariablesDict.find(id);
	if (ite == mVariablesDict.end()) return;
	UINT rootSigPos = ite->second;
	ComputeShaderVariable& var = mVariablesVector[rootSigPos];
	UploadBuffer* uploadBufferPtr;
	switch (var.type)
	{
	case ComputeShaderVariable::Type::SRVDescriptorHeap:
		commandList->SetComputeRootDescriptorTable(
			rootSigPos,
			((DescriptorHeap*)targetObj)->hGPU(indexOffset)
		);
		break;
	case ComputeShaderVariable::Type::UAVDescriptorHeap:
		commandList->SetComputeRootDescriptorTable(
			rootSigPos,
			((DescriptorHeap*)targetObj)->hGPU(indexOffset)
		);
		break;
	case ComputeShaderVariable::Type::ConstantBuffer:
		uploadBufferPtr = ((UploadBuffer*)targetObj);
		commandList->SetComputeRootConstantBufferView(
			rootSigPos,
			uploadBufferPtr->Resource()->GetGPUVirtualAddress() + indexOffset * uploadBufferPtr->GetAlignedStride()
		);
		break;
	case ComputeShaderVariable::Type::StructuredBuffer:
		uploadBufferPtr = ((UploadBuffer*)targetObj);
		commandList->SetComputeRootShaderResourceView(
			rootSigPos,
			uploadBufferPtr->Resource()->GetGPUVirtualAddress() + indexOffset * uploadBufferPtr->GetStride());
		break;
	}
}

void ComputeShader::SetStructuredBufferByAddress(ID3D12GraphicsCommandList* commandList, UINT id, D3D12_GPU_VIRTUAL_ADDRESS address)
{
	auto&& ite = mVariablesDict.find(id);
	if (ite == mVariablesDict.end()) return;
	UINT rootSigPos = ite->second;
	ComputeShaderVariable& var = mVariablesVector[rootSigPos];
	if (var.type == ComputeShaderVariable::Type::RWStructuredBuffer)
	{
		commandList->SetComputeRootUnorderedAccessView(
			rootSigPos,
			address);
	}
	else if (var.type == ComputeShaderVariable::Type::StructuredBuffer)
	{
		commandList->SetComputeRootShaderResourceView(
			rootSigPos,
			address
		);
	}
}

void ComputeShader::Dispatch(ID3D12GraphicsCommandList* commandList, UINT kernel, UINT x, UINT y, UINT z)
{
	commandList->SetPipelineState(pso[kernel].Get());
	commandList->Dispatch(x, y, z);
}

void ComputeShader::DispatchIndirect(ID3D12GraphicsCommandList* commandList, UINT dispatchKernel, StructuredBuffer* indirectBuffer, UINT bufferElement, UINT bufferIndex)
{
	commandList->SetPipelineState(pso[dispatchKernel].Get());
	commandList->ExecuteIndirect(mCommandSignature.Get(), 1, indirectBuffer->GetResource(), indirectBuffer->GetAddressOffset(bufferElement, bufferIndex), nullptr, 0);
}