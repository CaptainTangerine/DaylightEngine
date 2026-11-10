#pragma once
static const float PI = 3.14159265359;
// Normal Deistribution Function (D)
// NDF throwbridge-ReitZ GGX
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.f);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.f) + 1.f);
    denom = PI * denom * denom;
    
    return num / denom;
}
// Fresnel Reflectance (F)
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

//  Geometry Function (G)
float GeometrySchlickGGX(float3 N, float3 V, float roughness)
{
    float r = (roughness + 1.f);
    float k = (r * r) / 8.f;
    
    float NdotV = max(dot(N, V), 0.f);
    float denom = NdotV * (1 - k) + k;
    
    return NdotV / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float ggx2 = GeometrySchlickGGX(N, V, roughness);
    float ggx1 = GeometrySchlickGGX(N, L, roughness);

    return ggx1 * ggx2;
}
