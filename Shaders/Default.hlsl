//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

// Defaults for number of lights.
// Include structures and functions for lighting.
#include "LightingUtil.hlsl"
//Texture2D<float4> _MainTex[10] : register(t0, space0);

SamplerState pointWrapSampler  : register(s0);
SamplerState pointClampSampler  : register(s1);
SamplerState linearWrapSampler  : register(s2);
SamplerState linearClampSampler  : register(s3);
SamplerState anisotropicWrapSampler  : register(s4);
SamplerState anisotropicClampSampler  : register(s5);

cbuffer Per_Object_Buffer : register(b0)
{
    float4x4 gWorld;
};

cbuffer Per_Camera_Buffer : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 worldSpaceCameraPos;
    float gNearZ;
    float gFarZ;
};

cbuffer Per_Material_Buffer : register(b2)
{
	float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float  gRoughness;
    float4x4 gMatTransform;
};

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul((float3x3)gWorld, vin.NormalL);

    // Transform to homogeneous clip space.
    vout.PosH = mul(gViewProj, posW);
    //vout.PosH /= vout.PosH.w;
    //vout.PosH.z = 1 - vout.PosH.z;
    vout.TexC.xy = vin.TexC.xy;
    return vout;
}

void PS(VertexOut pin, out float4 albedo : SV_TARGET0, out float4 specular : SV_TARGET1, out float4 normal : SV_TARGET2, out float4 emission : SV_TARGET3, out float2 mv : SV_TARGET4)
{
   // float2 bindlessChooser = floor(saturate(pin.TexC) * 3);
 //   uint sampleCount = (uint)(bindlessChooser.x * 3 + bindlessChooser.y);
    albedo = 0;
    specular = 0;
    normal = 0;
    emission = 0.5;
    mv = 0;
}

float4 VS_Depth(float3 position : POSITION) : SV_POSITION
{
    float4 posW = mul(gWorld, float4(position, 1));
    return mul(gViewProj, posW);
}

void PS_Depth(){}