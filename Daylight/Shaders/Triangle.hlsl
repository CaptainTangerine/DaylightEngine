struct VS_Input
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct VS_Output
{
    float4 position : SV_Position;
    float3 color    : COLOR;
};

VS_Output VS_Main(VS_Input input)
{
    VS_Output output;
    output.position = float4(input.position, 1.f);
    output.color = input.color;
    return output;
}

float4 PS_Main(VS_Output input ) : SV_Target 
{
    return float4(input.color, 1.f);

}
