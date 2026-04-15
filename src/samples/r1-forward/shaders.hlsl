

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

float3 fresnel(float HdotV, float3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

float NormalDistroGGX(float NdotH, float a)
{
    float a2 = a * a;
    float denom = 3.14159 * pow((NdotH * NdotH * (a2 - 1.0) + 1.0), 2.0);
    return a2 / denom;
}
float GeometrySchlickGGX(float x, float k)
{
    return x / (x * (1.0 - k) + k);
}
float GeometrySmith(float NdotV, float NdotL, float k)
{
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
    return ggx1 * ggx2;
}

float3 computeLightPbr(float3 position, float3 N, float3 eye, Light light, MaterialValues material)
{
    float pi = 3.14159;
    float3 l = light.position - position;
    float  d = length(l);
    float3 L = l / d;
    float3 V = normalize(eye - position);
    float3 H = normalize(L + V);

    float  attenuation = (light.fallOff1 - d) / (light.fallOff1 - light.fallOff0);
    float  spot = pow(max(dot(-L, light.direction), 0), light.spotPower);
    float3 lightValue = attenuation * spot * light.intensity;

    float  HdotV = saturate(dot(H, V));
    float  NdotV = saturate(dot(N, V));
    float  NdotH = saturate(dot(N, H));
    float  NdotL = saturate(dot(N, L));
    
    float  rou = material.mr.x;
    float  met = material.mr.y;
    float  k = pow(rou + 1.0, 2.0) / 8.0;
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), material.bc.rgb, met);
    float3 ks = fresnel(HdotV, F0);
    float3 kd = 1 - ks;
    float3 f = kd * material.bc.rgb / pi + ks * NormalDistroGGX(NdotH, rou) * GeometrySmith(NdotV, NdotL, k) / max(4.0 * NdotV * NdotL, 0.001);

    float3 Lo = f * NdotL * lightValue;
    return Lo;
}

float3 computeLight(float3 position, float3 N, float3 eye, Light light, MaterialValues material)
{
    float pi = 3.14159;
    float3 l = light.position - position;
    float  d = length(l);
    float3 L = l / d;
    float3 V = normalize(eye - position);

    float  NdotL = saturate(dot(N, L));
    float attenuation = (light.fallOff1 - d) / (light.fallOff1 - light.fallOff0);
    float spot = pow(max(dot(-L, light.direction), 0), light.spotPower);
    float3 lightValue = attenuation * spot * light.intensity;

    float3 f = material.bc.rgb;
    float3 Lo = f * NdotL * lightValue;
    return Lo;
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
    material.mr = float2(metallic, roughness) * textures[metallicRoughnessMap].Sample(samplerLinearWrap, input.uv).gb;
    material.e  = emissive                    * textures[emissiveMap]         .Sample(samplerLinearWrap, input.uv).rgb;
    material.n  = 2                           * textures[normalMap]           .Sample(samplerLinearWrap, input.uv).rgb - 1;
    material.o  =                               textures[occlusionMap]        .Sample(samplerLinearWrap, input.uv).r;
// material.bc = pow(material.bc, 2.2);

    float3 ambient = float3(.05, .05, .05);
    float3 light = float3(0, 0, 0);
    light += ambient * material.bc.rgb + material.e;
    for (int i = 0; i < 3; i++) {
        // light += computeLight(input.positionW, normal, eye, lights[i], material);
        light += computeLightPbr(input.positionW, normal, eye, lights[i], material);
    }

    return float4(light, material.bc.a);
    // return diffuseMapValue;
}