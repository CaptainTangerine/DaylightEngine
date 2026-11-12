#include "Common/Math.hlsli"

Texture2D<float4> EquirectangularTexture;
SamplerState LinearSampler;

struct FaceIndex
{
	uint idx;
};

[[vk::push_constant]]
ConstantBuffer<FaceIndex> PushConstant;

struct VSOutput
{
	float4 Position : SV_Position;
	float2 Facecoord : TEXCOORD0;
};

VSOutput VS_Main(uint vertexID : SV_VertexID)
{
	static const float2 Positions[3] =
	{
		float2(-1.0, -1.0),
        float2(3.0, -1.0),
        float2(-1.0, 3.0)
	};
	
	VSOutput output;
	output.Position = float4(Positions[vertexID], 0.f, 1.f);
	output.Facecoord = Positions[vertexID];
	
	return output;
}

float3 GetCubemapDirection(uint face, float2 st)
{
	switch (face)
	{
		case 0:
			return normalize(float3(1.0, st.y, -st.x)); // +X
		case 1:
			return normalize(float3(-1.0, st.y, st.x)); // -X
		case 2:
			return normalize(float3(st.x, 1.0, -st.y)); // +Y
		case 3:
			return normalize(float3(st.x, -1.0, st.y)); // -Y
		case 4:
			return normalize(float3(st.x, st.y, 1.0)); // +Z
		default:
			return normalize(float3(-st.x, st.y, -1.0)); // -Z
	}
}

float2 DirectionToEquirectangularUV(float3 direction)
{
	direction = normalize(direction);

	float2 uv;
	uv.x = atan2(direction.z, direction.x) / (2.f * PI) + 0.5;
	uv.y = acos(clamp(direction.y, -1.0, 1.0)) / PI;

	return uv;
}


float4 PS_Main(VSOutput input) : SV_Target0
{
	float3 direction = GetCubemapDirection(PushConstant.idx, input.Facecoord);

	float2 uv = DirectionToEquirectangularUV(direction);

	return EquirectangularTexture.Sample(LinearSampler, uv);
}
