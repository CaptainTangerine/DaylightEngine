#pragma once

struct VSInput
{
    float3 inPosition : POSITION;
    float3 inColor : COLOR0;
    float3 inNormal : NORMAL0;
    float2 inTexcoord : TEXCOORD0;
    float4 inTangent : TANGENT0;
};
