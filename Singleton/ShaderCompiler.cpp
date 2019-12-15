#include "ShaderCompiler.h"
std::unordered_map<std::string, Shader*> ShaderCompiler::mShaders;
void ShaderCompiler::AddShader(std::string str, Shader* shad)
{
	mShaders[str] = shad;
}

Shader* ShaderCompiler::GetShader(std::string name)
{
	return mShaders[name];
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
	p.filePath = L"Shaders\\Default.hlsl";
	p.name = "OpaqueStandard";
	p.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	p.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	p.depthStencilState = dsDesc;
	p.psShader = nullptr;
	p.vsShader = nullptr;
	Pass& sp = allPasses[1];
	sp.fragment = "PS_PureColor";
	sp.vertex = "VS";
	sp.filePath = L"Shaders\\Default.hlsl";
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
	Shader* opaqueShader = new Shader(allPasses, var, device);
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
	p.filePath = L"Shaders\\Skybox.hlsl";
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
	Shader* skyboxShader = new Shader(allPasses, var, device);
	ShaderCompiler::AddShader("Skybox", skyboxShader);
}

void ShaderCompiler::Init(ID3D12Device* device)
{
	mShaders.reserve(50);
	GetOpaqueStandardShader(device);
	GetSkyboxShader(device);
}

