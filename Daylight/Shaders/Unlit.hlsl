#include "Common/VertexInput.hlsli"
#include "Common/PerView.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    float4 position = float4(input.inPosition, 1.0f);
    position = mul(position, viewUBO.viewMatrix);
    position = mul(position, viewUBO.projMatrix);
    
    output.position = position;
    output.color = input.inColor;
    return output;
}

float4 PS_Main(VSOutput input) : SV_Target0
{
    return float4(input.color, 1.f);
}
