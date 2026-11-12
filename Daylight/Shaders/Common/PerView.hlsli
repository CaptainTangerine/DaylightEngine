#pragma once

struct PerViewUniform
{
    row_major float4x4 viewMatrix;
    row_major float4x4 projMatrix;
	row_major float4x4 inverseViewMatrix;
	row_major float4x4 inverseProjectionMatrix;

    float3 cameraPosition;
};

ConstantBuffer<PerViewUniform> viewUBO;
