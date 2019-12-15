#pragma once
#include <unordered_map>
#include <string>
#include "../RenderComponent/Shader.h"
class ShaderCompiler
{
private:
	static std::unordered_map<std::string, Shader*> mShaders;
public:
	static Shader* GetShader(std::string name);
	static void AddShader(std::string str, Shader* shad);
	static void Init(ID3D12Device* device);
};