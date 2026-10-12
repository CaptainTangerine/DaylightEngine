[[vk::binding(0, 0)]] Texture2D<float4> textures[64];
[[vk::binding(0, 1)]] SamplerState imageSampler;

struct VSInput
{
    [[vk::location(0)]] float2 inPosition : POSITION;
    [[vk::location(1)]] float2 inTexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    output.position = float4(input.inPosition, 0.0f, 1.0f);
    output.uv = input.inTexCoord;
    return output;
}

float4 PS_Main(VSOutput input) : SV_Target0
{
    return textures[0].Sample(imageSampler, input.uv);

}
