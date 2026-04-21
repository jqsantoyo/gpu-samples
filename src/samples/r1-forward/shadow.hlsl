

struct Light {
    float3 position;
    float  fallOff0;
    float3 direction;
    float  fallOff1;
    float3 intensity;
    float  spotPower;
};


cbuffer ObjectData : register(b0)
{
    float4x4 m;
};

cbuffer MaterialData : register(b1)
{
    float4  baseColor;
    float3  emissive;
    float   metallic;
    float   roughness;
    int     baseColorMap;
    int     ormMap;
    int     normalMap;
    int     emissiveMap;
};

cbuffer PassData : register(b2)
{
    float4x4    view;
    float4x4    proj;
    float4x4    viewProj;
    float3      eye;
    float       pad0;
    float4x4    lightViewProj;
    Light       lights[12];
    float2      screenSize;
    float       deltaTime;
    float       absoluteTime;
    int         shadowTexture;
};

Texture2D    textures[]        : register(t0);
SamplerState samplerLinearWrap : register(s0);

struct VSInput
{
    float3 position  : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
};



PSInput VSMain(VSInput input)
{

    PSInput output;
    float3 positionW = mul(m, float4(input.position, 1)).xyz;
    output.position  = mul(lightViewProj, float4(positionW, 1));
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1, 1, 1, 1);
}