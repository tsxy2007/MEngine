TextureCube cubemap : register(t0, space0);
SamplerState gsamLinear  : register(s4);
cbuffer Per_Camera_Buffer : register(b0)
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


struct appdata
{
	float3 vertex    : POSITION;
};

struct v2f
{
	float4 position    : SV_POSITION;
    float3 worldView : TEXCOORD0;
};

v2f vert(appdata v)
{
    v2f o;
    o.position = float4(v.vertex.xy, 0, 1);
    float4 worldPos = mul(gInvViewProj, float4(v.vertex.xy, 1, 1));
    worldPos.xyz /= worldPos.w;
    o.worldView = worldPos.xyz - worldSpaceCameraPos;
    return o;
}

float4 frag(v2f i) : SV_TARGET
{
    return cubemap.SampleLevel(gsamLinear, i.worldView, 0);
}