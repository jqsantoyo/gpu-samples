

struct Light {
    float3 position;
    float  fallOff0;
    float3 direction;
    float  fallOff1;
    float3 intensity;
    float  spotPower;
};

struct MaterialValues {
    float4 bc;
    float2 mr;
    float3 e;
    float3 n;
    float  o;
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
    int     metallicRoughnessMap;
    int     normalMap;
    int     emissiveMap;
    int     occlusionMap;
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

Texture2D    textures[]        : register(t0);
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

float3 computeLight(float3 position, float3 normal, float3 eye, Light light, MaterialValues material)
{
    float3 v = eye - position;
    float3 L = light.position - position;
    float d = length(L);
    float3 Ldir = L / d;
    float3 intensity = max(dot(Ldir, normal), 0) * light.intensity;

    float3 ambient = float3(.01, .01, .01);
    float attenuation = (light.fallOff1 - d) / (light.fallOff1 - light.fallOff0);
    float spot = pow(max(dot(-Ldir, light.direction), 0), light.spotPower);
    float3 lightValue = ambient + attenuation * spot * intensity;

    float3 color = lightValue * material.bc.rgb + material.e;
    return color;
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
    MaterialValues material;
    material.bc = baseColor                   * textures[baseColorMap]        .Sample(samplerLinearWrap, input.uv);
    material.mr = float2(metallic, roughness) * textures[metallicRoughnessMap].Sample(samplerLinearWrap, input.uv).xy;
    material.e  = emissive                    * textures[emissiveMap]         .Sample(samplerLinearWrap, input.uv).xyz;
    material.n  = 2                           * textures[normalMap]           .Sample(samplerLinearWrap, input.uv).xyz - 1;
    material.o  =                               textures[occlusionMap]        .Sample(samplerLinearWrap, input.uv).x;

    float3 light = float3(0, 0, 0);
    for (int i = 0; i < 3; i++) {
        light += computeLight(input.positionW, input.normalW, eye, lights[i], material);
    }
    return float4(light, material.bc.a);
    // return diffuseMapValue;
}