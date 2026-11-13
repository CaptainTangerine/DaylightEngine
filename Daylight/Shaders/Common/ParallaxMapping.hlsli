#pragma once

float2 ParallaxMapping(
    Texture2D<float4> heightTexture,
    SamplerState materialSampler,
    float2 texcoords,
    float3 tangentViewDir)
{
    const float minLayers = 8.0f;
    const float maxLayers = 32.0f;
    float numLayers = lerp(maxLayers, minLayers, saturate(dot(float3(0.0f, 0.0f, 1.0f), tangentViewDir)));
    float layerDepth = 1.0f / numLayers;

    const float heightScale = 0.02f;

    float viewZ = saturate(tangentViewDir.z);
    float safeViewZ = max(viewZ, 0.1f);

    float2 p = tangentViewDir.xy / safeViewZ * heightScale;
    float2 deltaTexcoord = p / numLayers;

    float currentLayerDepth = 0.0;
    float currentDepthMapValue = 1.0f - heightTexture.Sample(materialSampler, texcoords).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        texcoords -= deltaTexcoord;
        currentDepthMapValue = 1.0f - heightTexture.Sample(materialSampler, texcoords).r;

        currentLayerDepth += layerDepth;
    }

    float2 previousTexcoords = texcoords + deltaTexcoord;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = 1.0f - heightTexture.Sample(materialSampler, previousTexcoords).r - currentLayerDepth + layerDepth;
    float weight = saturate(afterDepth / (afterDepth - beforeDepth));

    return lerp(texcoords, previousTexcoords, weight);
}
