#include "Common/VertexInput.hlsli"
#include "Common/BRDF.hlsli"
#include "Common/ParallaxMapping.hlsli"
#include "Common/PerView.hlsli"

struct LightUniform
{
    float4 LightPos[4];
    float4 LightColor[4];
};

ConstantBuffer<LightUniform> LightUBO;

struct Material
{
    float3 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float aoFactor;
};

[[vk::push_constant]]
ConstantBuffer<Material> material;

Texture2D<float4> baseColorTexture;
Texture2D<float4> normalTexture;
Texture2D<float4> heightTexture;
Texture2D<float4> metallicTexture;
Texture2D<float4> roughnessTexture;
Texture2D<float4> ambientOcclusionTexture;

SamplerState materialSampler;
TextureCube<float4> irradianceMap;
SamplerState irradianceSampler;

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
    
    float3 tangentWorldPosition : Position0;
    float3 tangentViewPosition : Position1;
    float3 tangentLightPosition[4] : Position2;
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    
    float4 worldPosition = float4(input.inPosition, 1.0f);
    float4 clipPosition = mul(worldPosition, viewUBO.viewMatrix);
    clipPosition = mul(clipPosition, viewUBO.projMatrix);
    
    output.position = clipPosition;
    output.color = float4(input.inColor, 1.0f);
    output.texcoord = input.inTexcoord;
    
    // 현재 Model 행렬은 Identity
    // TODO : Model 행렬 도입시 Model Transpose의 역행렬 곱하기
    float3 T = normalize(input.inTangent.xyz);
    float3 N = normalize(input.inNormal);
    
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T) * input.inTangent.w;
    float3x3 TBN = float3x3(T, B, N);
    float3x3 invTBN = transpose(TBN);
    
    output.tangentWorldPosition = mul(worldPosition.xyz, invTBN);
    output.tangentViewPosition = mul(viewUBO.cameraPosition.xyz, invTBN);
    for (int i = 0; i < 4; ++i)
    {
        output.tangentLightPosition[i] = mul(LightUBO.LightPos[i].xyz, invTBN);
    }
    
    return output;
}
// 탄젠트 공간에서 빛 공간
float4 PS_Main(VSOutput input) : SV_Target0
{
    
    float3 V = normalize(input.tangentViewPosition - input.tangentWorldPosition);
    
    float2 displacedTexCoord = ParallaxMapping(heightTexture, materialSampler, input.texcoord, V);
 
    float3 N = normalTexture.Sample(materialSampler, displacedTexCoord).xyz * 2.0f - 1.0f;
    N = normalize(N);
    
    float3 baseColor = material.baseColorFactor * baseColorTexture.Sample(materialSampler, displacedTexCoord).rgb;
    float metallic = material.metallicFactor * metallicTexture.Sample(materialSampler, displacedTexCoord).r;
    float roughness = material.roughnessFactor * roughnessTexture.Sample(materialSampler, displacedTexCoord).r;
    float ao = material.aoFactor * ambientOcclusionTexture.Sample(materialSampler, displacedTexCoord).r;
    
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, baseColor, metallic);
    
    float3 Lo = float3(0.f, 0.f, 0.f);
    for (int i = 0; i < 4; ++i)
    {
        float3 L = normalize(input.tangentLightPosition[i] - input.tangentWorldPosition);
        float3 H = normalize(V + L);
        
        float distance = length(input.tangentLightPosition[i]- input.tangentWorldPosition);
        float attenuation = 1.f / (distance * distance);
        float3 radiance = LightUBO.LightColor[i].xyz * attenuation;
        
        float NDF = DistributionGGX(N, H, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.f), F0);
        float G = GeometrySmith(N, V, L, roughness);
        
        float3 ks = F;
        float3 kd = float3(1.f, 1.f, 1.f) - ks;
       
        kd *= (1.f - metallic);
        
        float3 nume = NDF * F * G;
        float3 denom = 4 * max(dot(N, V), 0.f) * max(dot(N, L), 0.f) + 0.0001;
        
        float3 specular = nume / denom;
        float NdotL = max(dot(N, L), 0.f);
        
        Lo += (kd * baseColor / PI + specular) * radiance * NdotL;
    }
    

    float3 kS = FresnelSchlickRoughness(max(dot(N, V), 0.f), F0, roughness);
    float3 kD = (1.0f - kS);
    float3 irradiance = irradianceMap.SampleLevel(irradianceSampler, N, 0).rgb;
    float3 diffuse = irradiance * baseColor;
    float3 ambient = (kD * diffuse) * ao;
    float3 color = ambient + Lo;
   
    return float4(color, 1.f);
}
