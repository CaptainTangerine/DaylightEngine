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

struct PerViewUniform
{
    row_major float4x4 viewMatrix;
    row_major float4x4 projMatrix;
};

ConstantBuffer<PerViewUniform> viewUBO;

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
