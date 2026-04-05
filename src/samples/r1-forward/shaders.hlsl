

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
    float4 albedoColor;
    float R0;
    float roughness;
};

cbuffer PassData : register(b2)
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewProj;
    float3 eye;
    float pad0;
    Light lights[12];
    float2 screenSize;
    float deltaTime;
    float absoluteTime;
};

Texture2D diffuseMap : register(t0);
SamplerState samplerLinearWrap : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float2 uv : UV;
};

float3 computeLight(float3 position, float3 normal, float3 eye, Light light, float3 albedo, float R0, float roughness)
{
    float3 v = eye - position;
    float3 L = light.position - position;
    float d = length(L);
    float3 Ldir = L / d;
    float3 intensity = max(dot(Ldir, normal), 0) * light.intensity;

    float attenuation = (light.fallOff1 - d) / (light.fallOff1 - light.fallOff0);
    float spot = pow(max(dot(-Ldir, light.direction), 0), light.spotPower);
    float3 value = attenuation * spot * intensity * albedo;
    return value;
}

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.positionW = mul(m, float4(input.position, 1)).xyz;
    output.normalW = mul((float3x3)m, input.normal);
    output.position = mul(viewProj, float4(output.positionW, 1));
    output.uv = input.uv; 
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normalW);
    float4 diffuseMapValue = diffuseMap.Sample(samplerLinearWrap, input.uv);
    float4 albedo = albedoColor * diffuseMapValue;
    float3 light = computeLight(input.positionW, input.normalW, eye, lights[0], albedo.rgb, R0, roughness);
    return float4(light, albedo.a);
}