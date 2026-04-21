

struct Light {
    float3 position;
    float  fallOff0;
    float3 direction;
    float  fallOff1;
    float3 intensity;
    float  spotPower;
};

struct MaterialValues {
    float3 bc;
    float2 mr;
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
    int         shadowMap;
};

Texture2D    textures[]        : register(t0);
SamplerState samplerAniso : register(s0);
SamplerState samplerShadow : register(s1);

struct VSInput
{
    float3 position  : POSITION;
    float3 normal    : NORMAL;
    float2 uv        : UV;
    float4 tangent   : TANGENT;
};

struct PSInput
{
    float4   position  : SV_POSITION;
    float3   positionW : POSITION;
    float4   positionL : POSITION_LIGHT;
    float2   uv        : UV;
    float3x3 TBN       : TBN;
};



float3 srgbToLinear(float3 x) {
    return lerp(x / 12.92, pow((x + 0.055) / 1.055, 2.4), step(0.04045, x));
}

float3 linearToSrgb(float3 x) {
    return lerp(x * 12.92, 1.055 * pow(x, 1.0/2.4) - 0.055, step(0.0031308, x));
}

// float3 srgbToLinear(float3 x) {
//     return x;
// }

// float3 linearToSrgb(float3 x) {
//     return x;
// }



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
    float  spot = pow(max(dot(-L, light.direction), 0.0), light.spotPower);
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
    float3 kd = (1.0 - ks) * (1.0 - met);
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
    float3 bitangent = cross(input.normal, input.tangent.xyz) * input.tangent.a;
    float3 T = normalize(mul((float3x3)m, input.tangent.xyz));
    float3 B = normalize(mul((float3x3)m, bitangent));
    float3 N = normalize(mul((float3x3)m, input.normal));

    PSInput output;
    output.positionW = mul(m, float4(input.position, 1)).xyz;
    output.position  = mul(viewProj, float4(output.positionW, 1));
    output.uv        = input.uv;
    output.TBN       = transpose(float3x3(T, B, N));

    output.positionL = mul(lightViewProj, float4(output.positionW, 1));
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 basecolorSample = textures[baseColorMap].Sample(samplerAniso, input.uv);
    float4       ormSample = textures[      ormMap].Sample(samplerAniso, input.uv);
    float4  emissiveSample = textures[ emissiveMap].Sample(samplerAniso, input.uv);
    float4    normalSample = textures[   normalMap].Sample(samplerAniso, input.uv);

    float3 shadowPos = input.positionL.xyz / input.positionL.w;
    float2 shadowUV = float2(shadowPos.x * .5 + .5, - shadowPos.y * .5 + .5);
    float4 shadowSample = textures[shadowMap].Sample(samplerShadow, shadowUV.xy);
    float shadowFactor = shadowPos.z <= (shadowSample.r + .0001) ? 1 : .4f;

    MaterialValues material;
    material.bc = baseColor.rgb * srgbToLinear(basecolorSample.rgb);
    material.mr = float2(roughness, metallic) * ormSample.gb;

    float  a  = baseColor.a * basecolorSample.a;
    float3 e  = emissive * srgbToLinear(emissiveSample.rgb);
    float  ao = ormSample.r;
    float3 N  = mul(input.TBN, 2 * normalSample.xyz - 1);

    float3 ambient = float3(.05, .05, .05);
    float3 light = float3(0, 0, 0);
    light += ambient * ao * material.bc + e;
    for (int i = 0; i < 3; i++) {
        // light += computeLight(input.positionW, normal, eye, lights[i], material);
        light += 1.2 * shadowFactor * computeLightPbr(input.positionW, N, eye, lights[i], material);
    }
    light = linearToSrgb(light);

    return float4(light, a);
}