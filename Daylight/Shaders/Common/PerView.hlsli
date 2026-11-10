#pragma once

struct PerViewUniform
{
    row_major float4x4 viewMatrix;
    row_major float4x4 projMatrix;
    float3 cameraPosition;
};

ConstantBuffer<PerViewUniform> viewUBO;
