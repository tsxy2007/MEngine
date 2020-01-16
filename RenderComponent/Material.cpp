#include "Material.h"
#include "../Singleton/ShaderID.h"
#include "UploadBuffer.h"
#include "Texture.h"
#include "../RenderComponent/DescriptorHeap.h"
using namespace std;
using Microsoft::WRL::ComPtr;
Material::Material(
	Shader* shader,
	ObjectPtr<UploadBuffer>& propertyBuffer,
	UINT propertyBufferIndex
) : MObject()
{
	mPropertyIndex = propertyBufferIndex;
	mShader = shader;
	mPropertyBuffer = propertyBuffer;
}
Material::~Material()
{
	mPropertyBuffer = nullptr;
	mShader = nullptr;
	for (int i = 0; i < propertiesValue.size(); ++i)
	{
		propertiesValue[i].obj = nullptr;
	}
}

void Material::BindShaderResource(ID3D12GraphicsCommandList* commandList)
{
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

bool Material::SetProperty(UINT id, ObjectPtr<MObject> obj, ShaderVariableType type, UINT offsetIndex)
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

bool Material::SetBindlessResource(UINT id, UINT offsetIndex, DescriptorHeap* shaderResourceHeap)
{
	return SetProperty(id, shaderResourceHeap, ShaderVariableType_DescriptorHeap, offsetIndex);
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