#include "Material.h"
#include "../Singleton/ShaderID.h"
using namespace std;
using Microsoft::WRL::ComPtr;
Material::Material(
	Shader* shader,
	ObjectPtr<UploadBuffer>& propertyBuffer,
	UINT propertyBufferIndex,
	ObjectPtr<DescriptorHeap>& srvHeap
) : MObject()
{
	shaderResourceHeap = srvHeap;
	mPropertyIndex = propertyBufferIndex;
	mShader = shader;
	mPropertyBuffer = propertyBuffer;
}
Material::~Material()
{
	shaderResourceHeap = nullptr;
	mPropertyBuffer = nullptr;
	mShader = nullptr;
	for (int i = 0; i < propertiesValue.size(); ++i)
	{
		propertiesValue[i].obj = nullptr;
	}
}

void Material::BindShaderResource(ID3D12GraphicsCommandList* commandList)
{
	if (shaderResourceHeap != nullptr)
	{
		ID3D12DescriptorHeap* descriptorHeaps = shaderResourceHeap->Get().Get();
		commandList->SetDescriptorHeaps(1, &descriptorHeaps);
	}
	mShader->BindRootSignature(commandList);
	if (mPropertyBuffer != nullptr)
	{
		mShader->SetResource(commandList, 
			ShaderID::GetPerMaterialBufferID(), 
			mPropertyBuffer.operator->(),
			mPropertyIndex);
	}
	for (int i = 0; i < propertiesValue.size(); ++i)
	{
		MatProperty& prop = propertiesValue[i];
		mShader->SetResource(commandList, prop.id, prop.obj.operator->(), prop.indexOffset);
	}

}

bool Material::SetProperty(UINT id, ObjectPtr<MObject> obj, ShaderVariable::Type type, UINT offsetIndex)
{
	ShaderVariable var;
	if (mShader->TryGetShaderVariable(id, var) && var.type == type)
	{
		RemoveProperty(id);
		propertiesKey[id] = propertiesValue.size();
		MatProperty p;
		p.type = type;
		p.id = id;
		p.indexOffset = offsetIndex;
		p.obj = obj;
		propertiesValue.push_back(p);
		return true;
	}
	return false;
}

bool Material::SetBindlessResource(UINT id, UINT offsetIndex)
{
	return SetProperty(id, shaderResourceHeap.operator->(), ShaderVariable::Type::DescriptorHeap, offsetIndex);
}

void Material::RemoveProperty(UINT key)
{
	auto&& ite = propertiesKey.find(key);
	if (ite == propertiesKey.end()) return;
	propertiesKey.erase(key);
	if (ite->second != propertiesValue.size() - 1) {
		propertiesValue[ite->second] = propertiesValue[propertiesValue.size() - 1];
		propertiesValue[propertiesValue.size() - 1].obj = nullptr;
		propertiesValue.erase(propertiesValue.end() - 1);
		MatProperty* p = &propertiesValue[ite->second];
		propertiesKey[p->id] = ite->second;
	}
	else
	{
		propertiesValue[propertiesValue.size() - 1].obj = nullptr;
		propertiesValue.erase(propertiesValue.end() - 1);

	}
}