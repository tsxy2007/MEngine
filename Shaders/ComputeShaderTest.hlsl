RWTexture2D<float4> _MainTex : register(u0, space0);
[numthreads(8,8,1)]
void CSMain(uint2 id : SV_DISPATCHTHREADID)
{
    _MainTex[id] = float4(_MainTex[id].x, 0, 0, 1);
}