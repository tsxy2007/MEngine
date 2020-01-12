#include "ShaderCompiler.h"
#include "../RenderComponent/Shader.h"
#include "../RenderComponent/ComputeShader.h"
#include "../JobSystem/JobInclude.h"

std::unordered_map<std::string, Shader*> ShaderCompiler::mShaders;
std::unordered_map<std::string, ComputeShader*> ShaderCompiler::mComputeShaders;
void ShaderCompiler::AddShader(std::string str, Shader* shad)
{
	mShaders[str] = shad;
}

void ShaderCompiler::AddComputeShader(std::string str, ComputeShader* shad)
{
	mComputeShaders[str] = shad;
}

Shader* ShaderCompiler::GetShader(std::string name)
{
	return mShaders[name];
}

ComputeShader* ShaderCompiler::GetComputeShader(std::string name)
{
	return mComputeShaders[name];
}

void GetPostProcessShader(ID3D12Device* device, JobBucket* bucket)
{
	//ZWrite
	D3D12_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = FALSE;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsDesc.FrontFace = defaultStencilOp;
	dsDesc.BackFace = defaultStencilOp;
	//Cull
	D3D12_RASTERIZER_DESC cullDesc;
	cullDesc.FillMode = D3D12_FILL_MODE_SOLID;
	cullDesc.CullMode = D3D12_CULL_MODE_NONE;
	cullDesc.FrontCounterClockwise = FALSE;
	cullDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	cullDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	cullDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	cullDesc.DepthClipEnable = FALSE;
	cullDesc.MultisampleEnable = FALSE;
	cullDesc.AntialiasedLineEnable = FALSE;
	cullDesc.ForcedSampleCount = 0;
	cullDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	const UINT PASS_COUNT = 1;
	Pass allPasses[PASS_COUNT];
	Pass& p = allPasses[0];
	p.fragment = "frag";
	p.vertex = "vert";
	p.filePath = L"Shaders\\PostProcess.hlsl";
	p.name = "PostProcess";
	p.rasterizeState = cullDesc;
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;
	p.psShader = nullptr;
	p.vsShader = nullptr;
	const UINT SHADER_VAR_COUNT = 1;
	//Properties

	ShaderVariable var[SHADER_VAR_COUNT];
	ShaderVariable& v = var[0];
	v.name = "_MainTex";
	v.registerPos = 0;
	v.space = 0;
	v.tableSize = 1;
	v.type = ShaderVariableType_DescriptorHeap;
	Shader* opaqueShader = new Shader(allPasses, PASS_COUNT, var, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddShader("PostProcess", opaqueShader);
}

void GetOpaqueStandardShader(ID3D12Device* device, JobBucket* bucket)
{
	D3D12_DEPTH_STENCIL_DESC gbufferDsDesc;
	gbufferDsDesc.DepthEnable = TRUE;
	gbufferDsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	gbufferDsDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
	gbufferDsDesc.StencilEnable = FALSE;
	gbufferDsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	gbufferDsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	D3D12_DEPTH_STENCIL_DESC depthPrepassDesc;
	depthPrepassDesc.DepthEnable = TRUE;
	depthPrepassDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthPrepassDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	depthPrepassDesc.StencilEnable = FALSE;
	depthPrepassDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	depthPrepassDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	gbufferDsDesc.FrontFace = defaultStencilOp;
	gbufferDsDesc.BackFace = defaultStencilOp;
	const UINT PASS_COUNT = 2;
	Pass allPasses[PASS_COUNT];
	Pass& p = allPasses[0];
	p.fragment = "PS";
	p.vertex = "VS";
	p.filePath = L"Shaders\\Default.hlsl";
	p.name = "OpaqueStandard";
	p.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = gbufferDsDesc;
	p.psShader = nullptr;
	p.vsShader = nullptr;
	Pass& dp = allPasses[1];
	dp.fragment = "PS_Depth";
	dp.vertex = "VS_Depth";
	dp.filePath = L"Shaders\\Default.hlsl";
	dp.name = "OpaqueStandard_Depth";
	dp.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	dp.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	dp.depthStencilState = depthPrepassDesc;
	dp.psShader = nullptr;
	dp.vsShader = nullptr;
	const UINT SHADER_VAR_COUNT = 3;
	ShaderVariable var[SHADER_VAR_COUNT];
	var[0].type = ShaderVariableType_ConstantBuffer;
	var[0].name = "Per_Object_Buffer";
	var[0].registerPos = 0;
	var[0].space = 0;

	var[1].type = ShaderVariableType_ConstantBuffer;
	var[1].name = "Per_Camera_Buffer";
	var[1].registerPos = 1;
	var[1].space = 0;

	var[2].type = ShaderVariableType_ConstantBuffer;
	var[2].name = "Per_Material_Buffer";
	var[2].registerPos = 2;
	var[2].space = 0;
	Shader* opaqueShader = new Shader(allPasses, PASS_COUNT, var, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddShader("OpaqueStandard", opaqueShader);

}

void GetSkyboxShader(ID3D12Device* device, JobBucket* bucket)
{
	D3D12_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsDesc.FrontFace = defaultStencilOp;
	dsDesc.BackFace = defaultStencilOp;
	const UINT PASS_COUNT = 1;
	Pass allPasses[PASS_COUNT];
	Pass& p = allPasses[0];
	p.fragment = "frag";
	p.vertex = "vert";
	p.filePath = L"Shaders\\Skybox.hlsl";
	p.name = "Skybox";
	p.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;
	p.psShader = nullptr;
	p.vsShader = nullptr;
	const UINT SHADER_VAR_COUNT = 2;
	ShaderVariable var[SHADER_VAR_COUNT];
	var[0].type = ShaderVariableType_ConstantBuffer;
	var[0].name = "SkyboxBuffer";
	var[0].registerPos = 0;
	var[0].space = 0;

	var[1].type = ShaderVariableType_DescriptorHeap;
	var[1].registerPos = 0;
	var[1].space = 0;
	var[1].name = "cubemap";
	var[1].tableSize = 1;
	Shader* skyboxShader = new Shader(allPasses, PASS_COUNT, var, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddShader("Skybox", skyboxShader);
}

void GetTemporalAAShader(ID3D12Device* device, JobBucket* bucket)
{
	//ZWrite
	D3D12_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = FALSE;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsDesc.FrontFace = defaultStencilOp;
	dsDesc.BackFace = defaultStencilOp;
	//Cull
	D3D12_RASTERIZER_DESC cullDesc;
	cullDesc.FillMode = D3D12_FILL_MODE_SOLID;
	cullDesc.CullMode = D3D12_CULL_MODE_NONE;
	cullDesc.FrontCounterClockwise = FALSE;
	cullDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	cullDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	cullDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	cullDesc.DepthClipEnable = FALSE;
	cullDesc.MultisampleEnable = FALSE;
	cullDesc.AntialiasedLineEnable = FALSE;
	cullDesc.ForcedSampleCount = 0;
	cullDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	const UINT PASS_COUNT = 1;
	Pass allPasses[PASS_COUNT];
	Pass& p = allPasses[0];
	p.fragment = "frag";
	p.vertex = "vert";
	p.filePath = L"Shaders\\TemporalAA.hlsl";
	p.name = "TemporalAA";
	p.rasterizeState = cullDesc;
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;
	p.psShader = nullptr;
	p.vsShader = nullptr;
	const UINT SHADER_VAR_COUNT = 2;
	ShaderVariable vars[2];
	vars[0].type = ShaderVariableType_ConstantBuffer;
	vars[0].name = "TAAConstBuffer";
	vars[0].registerPos = 0;
	vars[0].space = 0;

	vars[1].type = ShaderVariableType_DescriptorHeap;
	vars[1].name = "_MainTex";
	vars[1].registerPos = 0;
	vars[1].space = 0;
	vars[1].tableSize = 6;
	Shader* taaShader = new Shader(allPasses, PASS_COUNT, vars, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddShader("TemporalAA", taaShader);
}

void GetCullingShader(ID3D12Device* device, JobBucket* bucket)
{
	const UINT KERNEL_COUNT = 2;
	std::string kernelNames[KERNEL_COUNT];
	kernelNames[0] = "CSMain";
	kernelNames[1] = "Clear";
	const UINT SHADER_VAR_COUNT = 5;
	ComputeShaderVariable vars[SHADER_VAR_COUNT];
	vars[0].name = "CBuffer";
	vars[0].type = ComputeShaderVariable::ConstantBuffer;
	vars[0].registerPos = 0;
	vars[0].space = 0;

	vars[1].name = "_InputBuffer";
	vars[1].type = ComputeShaderVariable::StructuredBuffer;
	vars[1].registerPos = 0;
	vars[1].space = 0;

	vars[2].name = "_OutputBuffer";
	vars[2].type = ComputeShaderVariable::RWStructuredBuffer;
	vars[2].registerPos = 0;
	vars[2].space = 0;

	vars[3].name = "_CountBuffer";
	vars[3].type = ComputeShaderVariable::RWStructuredBuffer;
	vars[3].registerPos = 1;
	vars[3].space = 0;

	vars[4].name = "_InputDataBuffer";
	vars[4].type = ComputeShaderVariable::StructuredBuffer;
	vars[4].registerPos = 1;
	vars[4].space = 0;
	ComputeShader* cs = new ComputeShader(L"Shaders\\Cull.compute", kernelNames, KERNEL_COUNT, vars, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddComputeShader("Cull", cs);
}

void GetTextureCopyShader(ID3D12Device* device, JobBucket* bucket)
{
	const UINT KERNEL_COUNT = 3;
	std::string kernelNames[KERNEL_COUNT];
	kernelNames[0] = "CopyToRGBA32";
	kernelNames[1] = "CopyToRGBA64";
	kernelNames[2] = "CopyToRGBAFloat";
	const UINT SHADER_VAR_COUNT = 5;
	ComputeShaderVariable vars[SHADER_VAR_COUNT];
	vars[0].name = "CopyData";
	vars[0].type = ComputeShaderVariable::ConstantBuffer;
	vars[0].registerPos = 0;
	vars[0].space = 0;

	vars[1].name = "_RGBA32InputBuffer";
	vars[1].type = ComputeShaderVariable::StructuredBuffer;
	vars[1].registerPos = 0;
	vars[1].space = 0;

	vars[2].name = "_RGBA64InputBuffer";
	vars[2].type = ComputeShaderVariable::StructuredBuffer;
	vars[2].registerPos = 1;
	vars[2].space = 0;

	vars[3].name = "_RGBAFloatInputBuffer";
	vars[3].type = ComputeShaderVariable::StructuredBuffer;
	vars[3].registerPos = 2;
	vars[3].space = 0;

	vars[4].name = "_MainTex";
	vars[4].type = ComputeShaderVariable::UAVDescriptorHeap;
	vars[4].registerPos = 0;
	vars[4].space = 0;
	ComputeShader* cs = new ComputeShader(L"Shaders\\TextureCopy.compute", kernelNames, KERNEL_COUNT, vars, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddComputeShader("TextureCopy", cs);
}
JobBucket bucket;
void ShaderCompiler::Init(ID3D12Device* device, JobSystem* jobSys)
{
	//JobSystem sys(10);
	bucket.SetJobSystem(jobSys);
	mShaders.reserve(50);
	mComputeShaders.reserve(50);
	GetOpaqueStandardShader(device, &bucket);
	GetSkyboxShader(device, &bucket);
	GetCullingShader(device, &bucket);
	GetTextureCopyShader(device, &bucket);
	GetPostProcessShader(device, &bucket);
	GetTemporalAAShader(device, &bucket);
	jobSys->ExecuteBucket(&bucket, 1);
}

