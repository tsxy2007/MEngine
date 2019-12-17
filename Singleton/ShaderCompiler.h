#pragma once
#include <unordered_map>
#include <string>

class Shader;
class ComputeShader;
class ID3D12Device;
class ShaderCompiler
{
private:
	static std::unordered_map<std::string, ComputeShader*> mComputeShaders;
	static std::unordered_map<std::string, Shader*> mShaders;
public:
	static Shader* GetShader(std::string name);
	static ComputeShader* GetComputeShader(std::string name);
	static void AddShader(std::string str, Shader* shad);
	static void AddComputeShader(std::string str, ComputeShader* shad);
	static void Init(ID3D12Device* device);
};