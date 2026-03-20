cbuffer ObjectData : register(b0)
{
    float4x4 mvp;
    float4 v;
};

Texture2D diffuse : register(t0);
SamplerState samplerLinearWrap : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float2 uv : UV;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : UV;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(mvp, float4(input.position, 1));
    output.uv = input.uv;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return diffuse.Sample(samplerLinearWrap, input.uv);
}