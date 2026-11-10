#include "Common/VertexInput.hlsli"
#include "Common/BRDF.hlsli"
#include "Common/PerView.hlsli"

struct Material
{
    float3 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float aoFactor;
};

[[vk::push_constant]]
ConstantBuffer<Material> material;

struct LightUniform
{
    float4 LightPos[4];
    float4 LightColor[4];
};

ConstantBuffer<LightUniform> LightUBO;

struct VSOutput
{
    float4 position : SV_Position;
    float3 worldPosition : Position;
    float3 normal : Normal;
    float4 color : COLOR0;
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    float4 worldPosition = float4(input.inPosition, 1.0f);
    float4 clipPosition = mul(worldPosition, viewUBO.viewMatrix);
    clipPosition = mul(clipPosition, viewUBO.projMatrix);
    
    output.position = clipPosition;
    output.normal = input.inNormal;
    output.worldPosition = worldPosition.xyz;
    output.color = float4(input.inColor, 1.0f);
    
    return output;
}

float4 PS_Main(VSOutput input) : SV_Target0
{
    float3 N = normalize(input.normal);
    float3 V = normalize(viewUBO.cameraPosition - input.worldPosition);
    
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, material.baseColorFactor, material.metallicFactor);
    
    float3 Lo = float3(0.f, 0.f, 0.f);
    for (int i = 0; i < 4; ++i)
    {
        float3 L = normalize(LightUBO.LightPos[i].xyz - input.worldPosition);
        float3 H = normalize(V + L);
        
        float distance = length(LightUBO.LightPos[i].xyz - input.worldPosition);
        float attenuation = 1.f / (distance * distance);
        float3 radiance = LightUBO.LightColor[i].xyz * attenuation;
        
        float NDF = DistributionGGX(N, H, material.roughnessFactor);
        float3 F = FresnelSchlick(max(dot(H, V), 0.f), F0);
        float G = GeometrySmith(N, V, L, material.roughnessFactor);
        
        float3 ks = F;
        float3 kd = float3(1.f, 1.f, 1.f) - ks;
       
        kd *= (1.f - material.metallicFactor);
        
        float3 nume = NDF * F * G;
        float3 denom = 4 * max(dot(N, V), 0.f) * max(dot(N, L), 0.f) + 0.0001;
        
        float3 specular = nume / denom;
        float NdotL = max(dot(N, L), 0.f);
        
        Lo += (kd * material.baseColorFactor / PI + specular) * radiance * NdotL;
    }
    
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * material.baseColorFactor * material.aoFactor;
    float3 color = ambient + Lo;
    
    color = color / (color + 1.0f);
   
    return float4(color, 1.f);
}
