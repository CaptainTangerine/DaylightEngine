#pragma once

static const float PI = 3.14159265359;

float2 DirectionToEquirectangularUV(float3 direction)
{
    direction = normalize(direction);

    float2 uv;
    uv.x = atan2(direction.z, direction.x) / (2.f * PI) + 0.5;
    uv.y = acos(clamp(direction.y, -1.0, 1.0)) / PI;

    return uv;
}

float3 GetCubemapDirection(uint face, float2 st)
{
    switch (face)
    {
        case 0:
            return normalize(float3(1.0, st.y, -st.x)); // +X
        case 1:
            return normalize(float3(-1.0, st.y, st.x)); // -X
        case 2:
            return normalize(float3(st.x, 1.0, -st.y)); // +Y
        case 3:
            return normalize(float3(st.x, -1.0, st.y)); // -Y
        case 4:
            return normalize(float3(st.x, st.y, 1.0)); // +Z
        default:
            return normalize(float3(-st.x, st.y, -1.0)); // -Z
    }
}
