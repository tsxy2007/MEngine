#include "CommandSignature.h"
#include "Shader.h"
#include "../Singleton/ShaderID.h"
CommandSignature::CommandSignature(Shader* shader, ID3D12Device* device) :
	mShader(shader)
{
	D3D12_COMMAND_SIGNATURE_DESC desc = {};
	D3D12_INDIRECT_ARGUMENT_DESC indDesc[5];
	ZeroMemory(indDesc, 5 * sizeof(D3D12_INDIRECT_ARGUMENT_DESC));
	indDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	indDesc[0].ConstantBufferView.RootParameterIndex = shader->GetPropertyRootSigPos(ShaderID::GetPerObjectBufferID());
	indDesc[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	indDesc[1].ConstantBufferView.RootParameterIndex = shader->GetPropertyRootSigPos(ShaderID::GetPerMaterialBufferID());
	indDesc[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
	indDesc[2].VertexBuffer.Slot = 0;
	indDesc[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
	indDesc[4].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	desc.ByteStride = sizeof(MultiDrawCommand);
	desc.NodeMask = 0;
	desc.NumArgumentDescs = 5;
	desc.pArgumentDescs = indDesc;
	ThrowIfFailed(device->CreateCommandSignature(&desc, shader->GetSignature(), IID_PPV_ARGS(&mCommandSignature)));
}