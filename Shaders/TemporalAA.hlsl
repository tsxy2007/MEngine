cbuffer TAAConstBuffer : register(b0)
{
	float4x4 _InvNonJitterVP;
	float4x4 _InvLastVp;
	float4 _FinalBlendParameters;
	float4 _CameraDepthTexture_TexelSize;
	//Align
	float3 _TemporalClipBounding;
	float _Sharpness;
	//Align
	float2 _Jitter;
	float2 _LastJitter;
	//Align
	uint mainTexIndex;
	uint lastRtTexIndex;
	uint lastDepthIndex;
	uint lastMotionIndex;
	//Align
	uint motionVectorIndex;
	uint depthTexIndex;
};

Texture2D<float4> _MainTex[6] : register(t0);
SamplerState pointWrapSampler  : register(s0);
SamplerState pointClampSampler  : register(s1);
SamplerState linearWrapSampler  : register(s2);
SamplerState linearClampSampler  : register(s3);
SamplerState anisotropicWrapSampler  : register(s4);
SamplerState anisotropicClampSampler  : register(s5);


        struct appdata
		{
			float4 vertex : POSITION;
			float2 texcoord : TEXCOORD0;
		};

		struct v2f
		{
			float4 vertex : SV_POSITION;
			float2 texcoord : TEXCOORD0;
		};

		v2f vert(appdata v)
		{
			v2f o;
			o.vertex = v.vertex;
			o.texcoord = v.texcoord;
			return o;
		}
        
    #ifndef AA_VARIANCE
    #define AA_VARIANCE 1
    #endif

    #ifndef AA_Filter
    #define AA_Filter 1
    #endif
    /*
    inline float2 LinearEyeDepth( float2 z )
    {
        return 1.0 / (_ZBufferParams.z * z + _ZBufferParams.w);
    }

    inline float2 Linear01Depth( float2 z )
{
    return 1.0 / (_ZBufferParams.x * z + _ZBufferParams.y);
}
    float Luma4(float3 Color)
    {
        return (Color.g * 2) + (Color.r + Color.b);
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    float3 RGBToYCoCg(float3 RGB)
    {
        const float3x3 mat = float3x3(0.25,0.5,0.25,0.5,0,-0.5,-0.25,0.5,-0.25);
        float3 col =mul(mat, RGB);
        return col;
    }
    
    float3 YCoCgToRGB(float3 YCoCg)
    {
        const float3x3 mat = float3x3(1,1,-1,1,0,1,1,-1,-1);
        return mul(mat, YCoCg);
    }

    float4 RGBToYCoCg(float4 RGB)
    {
        return float4(RGBToYCoCg(RGB.xyz), RGB.w);
    }



    float4 YCoCgToRGB(float4 YCoCg)
    {
        return float4(YCoCgToRGB(YCoCg.xyz), YCoCg.w); 
    }
    float Luma(float3 Color)
    {
        return (Color.g * 0.5) + (Color.r + Color.b) * 0.25;
    }
    #define TONE_BOUND 0.5
    float3 Tonemap(float3 x) 
    { 
        float luma = Luma(x);
        [flatten]
        if(luma <= TONE_BOUND) return x;
        else return x * (TONE_BOUND * TONE_BOUND - luma) / (luma * (2 * TONE_BOUND - 1 - luma));
        //return x * weight;
    }

    float3 TonemapInvert(float3 x) { 
        float luma = Luma(x);
        [flatten]
        if(luma <= TONE_BOUND) return x;
        else return x * (TONE_BOUND * TONE_BOUND - (2 * TONE_BOUND - 1) * luma) / (luma * (1 - luma));
    }

    float Pow2(float x)
    {
        return x * x;
    }

    float HdrWeight4(float3 Color, const float Exposure) 
    {
        return rcp(Luma4(Color) * Exposure + 4);
    }

    float3 ClipToAABB(float3 color, float3 minimum, float3 maximum)
    {
        // Note: only clips towards aabb center (but fast!)
        float3 center = 0.5 * (maximum + minimum);
        float3 extents = 0.5 * (maximum - minimum);

        // This is actually `distance`, however the keyword is reserved
        float3 offset = color.rgb - center;

        float3 ts = abs(extents / (offset + 0.0001));
        float t = saturate(Min3(ts.x, ts.y, ts.z));
        color.rgb = center + offset * t;
        return color;
    }

    static const int2 _OffsetArray[8] = {
        int2(-1, -1),
        int2(0, -1),
        int2(1, -1),
        int2(-1, 0),
        int2(1, 1),
        int2(1, 0),
        int2(-1, 1),
        int2(0, -1)
    };
    #define COMPARE_DEPTH(a, b) step(b, a)

float2 ReprojectedMotionVectorUV(float2 uv, out float outDepth)
    {
        float neighborhood;
        const float2 k = _CameraDepthTexture_TexelSize.xy;
        uint i;
        outDepth = _MainTex[depthTexIndex].SampleLevel(pointClampSampler, uv, 0).x;
        float3 result = float3(0, 0,  outDepth);
        [loop]
        for(i = 0; i < 8; ++i){
            neighborhood = _MainTex[depthTexIndex].SampleLevel(pointClampSampler, uv, 0, _OffsetArray[i]);
            result = lerp(result, float3(_OffsetArray[i], neighborhood), COMPARE_DEPTH(neighborhood, result.z));
        }
        return uv + result.xy * k;
    }

*/
float4 frag(v2f i) : SV_TARGET
{
   /* float depth;
    ReprojectedMotionVectorUV(i.texcoord, depth);
    return depth;*/
    return 0;
}