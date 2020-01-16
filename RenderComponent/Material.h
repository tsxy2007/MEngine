#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "Shader.h"
#include <vector>
#include <memory>
class DescriptorHeap;
class UploadBuffer;
class Material : public MObject
{
private:
	struct MatProperty
	{
		UINT id;
		ShaderVariableType type;
		ObjectPtr<MObject> obj = nullptr;
		UINT indexOffset;
	};
	UINT mPropertyIndex;
	Shader* mShader;
	ObjectPtr<UploadBuffer> mPropertyBuffer = nullptr;
	std::unordered_map<UINT, UINT> propertiesKey;
	std::vector<MatProperty> propertiesValue;
	bool SetProperty(UINT id, ObjectPtr<MObject> obj, ShaderVariableType type, UINT offsetIndex);
public:
	Material(
		Shader* shader,
		ObjectPtr<UploadBuffer>& propertyBuffer,
		UINT propertyBufferIndex
	);
	virtual ~Material();
	inline Shader* GetShader() const { return mShader; }
	void BindShaderResource(ID3D12GraphicsCommandList* commandList);
	bool SetBindlessResource(UINT id, UINT offsetIndex, DescriptorHeap* shaderResourceHeap);
	void RemoveProperty(UINT key);
};