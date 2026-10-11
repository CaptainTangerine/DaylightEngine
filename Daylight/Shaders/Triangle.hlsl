struct FVSInput
{
    [[vk::location(0)]] float2 inPosition : POSITION;
    [[vk::location(1)]] float3 inColor : COLOR0;
};

struct FVSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

FVSOutput VS_Main(FVSInput input)
{
    FVSOutput output;
    output.position = float4(input.inPosition, 0.0f, 1.0f);
    output.color = input.inColor;
    return output;
}

float4 PS_Main(FVSOutput input) : SV_Target0
{
    return float4(input.color, 1.f);
}
