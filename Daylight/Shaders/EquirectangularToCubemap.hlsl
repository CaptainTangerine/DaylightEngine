#include "Common/Math.hlsli"

Texture2D<float4> EquirectangularTexture;
SamplerState LinearSampler;

struct Face
{
	uint idx;
};

[[vk::push_constant]]
ConstantBuffer<Face> FacePS;

struct VSOutput
{
	float4 Position : SV_Position;
	float2 Facecoord : TEXCOORD0;
};

VSOutput VS_Main(uint vertexID : SV_VertexID)
{
	static const float2 Positions[3] =
	{
		float2(-1.f, -1.0),
        float2(3.f, -1.f),
        float2(-1.f, 3.f)
	};
	
	VSOutput output;
	output.Position = float4(Positions[vertexID], 0.f, 1.f);
	output.Facecoord = Positions[vertexID];
	
	return output;
}

float4 PS_Main(VSOutput input) : SV_Target0
{
    float3 direction = GetCubemapDirection(FacePS.idx, input.Facecoord);
	float2 uv = DirectionToEquirectangularUV(direction);

	return EquirectangularTexture.Sample(LinearSampler, uv);
}
