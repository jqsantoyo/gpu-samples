cbuffer ObjectData : register(b0)
{
    float4x4 mvp;
    float4 v;
};

struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(mvp, float4(input.position, 1));
    output.color = float4(input.color, 1);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1, 1, 1, 1);
}