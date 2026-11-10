Texture2D<float4> SceneColor;
SamplerState SceneSampler;

struct ToneMappingPushConstants
{
    float exposure;
};

[[vk::push_constant]]
ConstantBuffer<ToneMappingPushConstants> toneMapping;

struct VSOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

VSOutput VS_Main(uint vertexId : SV_VertexID)
{
    VSOutput output;
    output.texcoord = float2(vertexId & 2, (vertexId << 1) & 2);
    output.position = float4(output.texcoord.x * 2.0f - 1.0f, 1.0f - output.texcoord.y * 2.0f, 0.0f, 1.0f);
    return output;
}

float4 PS_Main(VSOutput input) : SV_Target0
{
    float3 color = SceneColor.Sample(SceneSampler, input.texcoord).rgb;
    color = 1.0f - exp(-color * toneMapping.exposure);
    return float4(color, 1.0f);
}
