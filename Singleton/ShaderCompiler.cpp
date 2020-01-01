#include "ShaderCompiler.h"
#include "../RenderComponent/Shader.h"
#include "../RenderComponent/ComputeShader.h"
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

void GetPostProcessShader(ID3D12Device* device)
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
	std::vector<Pass> allPasses(1);
	Pass& p = allPasses[0];
	p.fragment = "frag";
	p.vertex = "vert";
	p.filePath = L"Shaders\\PostProcess";
	p.name = "PostProcess";
	p.rasterizeState = cullDesc;
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;
	p.psShader = nullptr;
	p.vsShader = nullptr;
	//Properties
	std::vector<ShaderVariable> var(1);
	ShaderVariable& v = var[0];
	v.name = "_MainTex";
	v.registerPos = 0;
	v.space = 0;
	v.tableSize = 1;
	v.type = ShaderVariable::DescriptorHeap;
	Shader* opaqueShader = new Shader(allPasses, var, device, false);
	ShaderCompiler::AddShader("PostProcess", opaqueShader);
}

void GetOpaqueStandardShader(ID3D12Device* device)
{
	D3D12_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsDesc.FrontFace = defaultStencilOp;
	dsDesc.BackFace = defaultStencilOp;

	std::vector<Pass> allPasses(2);
	Pass& p = allPasses[0];
	p.fragment = "PS";
	p.vertex = "VS";
	p.filePath = L"Shaders\\Default";
	p.name = "OpaqueStandard";
	p.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;
	p.psShader = nullptr;
	p.vsShader = nullptr;
	Pass& sp = allPasses[1];
	sp.fragment = "PS_PureColor";
	sp.vertex = "VS";
	sp.filePath = L"Shaders\\Default";
	sp.name = "OpaqueStandardPureColor";
	sp.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	sp.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	sp.depthStencilState = dsDesc;
	sp.psShader = nullptr;
	sp.vsShader = nullptr;
	std::vector<ShaderVariable> var(5);
	var[0].type = ShaderVariable::Type::DescriptorHeap;
	var[0].registerPos = 2;
	var[0].space = 1;
	var[0].tableSize = 10;
	var[0].name = "gDiffuseMap";

	var[1].type = ShaderVariable::Type::ConstantBuffer;
	var[1].name = "Per_Object_Buffer";
	var[1].registerPos = 0;
	var[1].space = 0;

	var[2].type = ShaderVariable::Type::ConstantBuffer;
	var[2].name = "Per_Camera_Buffer";
	var[2].registerPos = 1;
	var[2].space = 0;

	var[3].type = ShaderVariable::Type::ConstantBuffer;
	var[3].name = "Per_Material_Buffer";
	var[3].registerPos = 2;
	var[3].space = 0;

	var[4].type = ShaderVariable::Type::DescriptorHeap;
	var[4].registerPos = 0;
	var[4].space = 0;
	var[4].name = "cubemap";
	var[4].tableSize = 1;
	Shader* opaqueShader = new Shader(allPasses, var, device, false);
	ShaderCompiler::AddShader("OpaqueStandard", opaqueShader);

}

void GetSkyboxShader(ID3D12Device* device)
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

	std::vector<Pass> allPasses(1);
	Pass& p = allPasses[0];
	p.fragment = "frag";
	p.vertex = "vert";
	p.filePath = L"Shaders\\Skybox";
	p.name = "Skybox";
	p.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;
	p.psShader = nullptr;
	p.vsShader = nullptr;
	std::vector<ShaderVariable> var(2); 
	var[0].type = ShaderVariable::Type::ConstantBuffer;
	var[0].name = "Per_Camera_Buffer";
	var[0].registerPos = 0;
	var[0].space = 0;

	var[1].type = ShaderVariable::Type::DescriptorHeap;
	var[1].registerPos = 0;
	var[1].space = 0;
	var[1].name = "cubemap";
	var[1].tableSize = 1;
	Shader* skyboxShader = new Shader(allPasses, var, device, false);
	ShaderCompiler::AddShader("Skybox", skyboxShader);
}

void GetCullingShader(ID3D12Device* device)
{
	std::vector<std::string> kernelNames(2);
	kernelNames[0] = "CSMain";
	kernelNames[1] = "Clear";
	std::vector<ComputeShaderVariable> vars(5);
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
	ComputeShader* cs = new ComputeShader(L"Shaders\\Cull", kernelNames, vars, device, false);
	ShaderCompiler::AddComputeShader("Cull", cs);
}

void ShaderCompiler::Init(ID3D12Device* device)
{
	mShaders.reserve(50);
	mComputeShaders.reserve(50);
	GetOpaqueStandardShader(device);
	GetSkyboxShader(device);
	GetCullingShader(device);
	GetPostProcessShader(device);
}

