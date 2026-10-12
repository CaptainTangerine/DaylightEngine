struct VSInput
{
    [[vk::location(0)]] float2 inPosition : POSITION;
    [[vk::location(1)]] float3 inColor : COLOR0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    output.position = float4(input.inPosition, 0.0f, 1.0f);
    output.color = input.inColor;
    return output;
}

float4 PS_Main(VSOutput input) : SV_Target0
{
    return float4(input.color, 1.f);
}
