#pragma once
#include "../Common/d3dUtil.h"
#include "Shader.h"
#include "UploadBuffer.h"
#include "Texture.h"
#include <vector>
#include <memory>
#include "../RenderComponent/DescriptorHeap.h"
class Material : public MObject
{
private:
	struct MatProperty
	{
		UINT id;
		ShaderVariable::Type type;
		ObjectPtr<MObject> obj = nullptr;
		UINT indexOffset;
	};
	UINT mPropertyIndex;
	Shader* mShader;
	ObjectPtr<UploadBuffer> mPropertyBuffer = nullptr;
	std::unordered_map<UINT, UINT> propertiesKey;
	std::vector<MatProperty> propertiesValue;
	ObjectPtr<DescriptorHeap> shaderResourceHeap = nullptr;

	bool SetProperty(UINT id, ObjectPtr<MObject> obj, ShaderVariable::Type type, UINT offsetIndex);
public:
	Material(
		Shader* shader,
		ObjectPtr<UploadBuffer>& propertyBuffer,
		UINT propertyBufferIndex,
		ObjectPtr<DescriptorHeap>& srvHeap
	);
	virtual ~Material();
	inline Shader* GetShader() const { return mShader; }
	void BindShaderResource(ID3D12GraphicsCommandList* commandList);
	bool SetBindlessResource(UINT id, UINT offsetIndex);
	void RemoveProperty(UINT key);
};