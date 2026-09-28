struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

static const float2 positions[3] =
{
    float2(0.0f, 0.5f),
    float2(-0.5f, -0.5f),
    float2(0.5f, -0.5f)
};

static const float4 colors[3] =
{
    float4(1.0f, 0.0f, 0.0f, 1.f),
    float4(0.0f, 1.0f, 0.0f, 1.f),
    float4(0.0f, 0.0f, 1.0f, 1.f)
};

VertexOutput VS_Main(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.position = float4(positions[vertexID], 0.0f, 1.0f);
    output.color = colors[vertexID];
    return output;
}

float4 PS_Main(VertexOutput input) : SV_Target0
{
    return input.color;
}
