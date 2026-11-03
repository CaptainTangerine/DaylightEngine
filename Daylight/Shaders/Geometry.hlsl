struct VSInput
{
    float3 inPosition : POSITION;
    float3 inColor : COLOR0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

struct TrasnformUniform
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};


VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    
    output.position = float4(input.inPosition, 1.0f);
    output.color = input.inColor;
    return output;
}

float4 PS_Main(VSOutput input) : SV_Target0
{
    return float4(input.color, 1.f);
}
