#include "ComputeShader.h"
#include "../Singleton/ShaderID.h"
#include "UploadBuffer.h"
#include "../Common/DescriptorHeap.h"
using namespace std;
using Microsoft::WRL::ComPtr;
ComputeShader::ComputeShader(
	wstring compilePath,
	vector<std::string>& kernelName,
	vector<ComputeShaderVariable>& allShaderVariables,
	ID3D12Device* device) : csShaders(kernelName.size()), pso(kernelName.size())
{
	mVariablesDict.reserve(allShaderVariables.size() + 2);
	mVariablesVector.reserve(allShaderVariables.size());
	for (int i = 0; i < allShaderVariables.size(); ++i)
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
	for (int i = 0; i < kernelName.size(); ++i)
	{
		csShaders[i] = d3dUtil::CompileShader(compilePath, nullptr, kernelName[i], "cs_5_1");
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
}

void ComputeShader::BindRootSignature(ID3D12GraphicsCommandList* commandList, DescriptorHeap* heap)
{
	commandList->SetComputeRootSignature(mRootSignature.Get());
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
		commandList->SetComputeRootUnorderedAccessView(
			rootSigPos,
			uploadBufferPtr->Resource()->GetGPUVirtualAddress() + indexOffset * uploadBufferPtr->GetStride());
		break;
	}
}

void ComputeShader::Dispatch(ID3D12GraphicsCommandList* commandList, UINT kernel, UINT x, UINT y, UINT z)
{
	commandList->SetPipelineState(pso[kernel].Get());
	commandList->Dispatch(x, y, z);
}