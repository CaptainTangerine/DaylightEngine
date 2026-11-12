#include "Common/PerView.hlsli"

TextureCube<float4> EnvironmentMap;
SamplerState EnvironmentSampler;

struct VSOutput
{
	float4 position : SV_Position;
	float2 ndc : TEXCOORD0;
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
	output.position = float4(Positions[vertexID], 0.0, 1.0);
	output.ndc = Positions[vertexID];
	return output;
}

float4 PS_Main(VSOutput input) : SV_Target0
{
	float4 viewPosition = mul(float4(input.ndc, 1.0, 1.0), viewUBO.inverseProjectionMatrix);
    float3 cameraPosition = float3(0.0, 0.0, 0.0);
    float3 farPosition = viewPosition.xyz / viewPosition.w;
	
    float3 viewDirection = normalize(farPosition - cameraPosition);
	
	float3 worldDirection = normalize(mul(float4(viewDirection, 0.0), viewUBO.inverseViewMatrix).xyz);
	
	// Cubemap은 월드 좌표 기준으로 만들어졌음..
	return EnvironmentMap.Sample(EnvironmentSampler, worldDirection);
}
