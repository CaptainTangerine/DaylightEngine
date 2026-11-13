#include "Common/Math.hlsli"

TextureCube<float4> EnvironmentCubemap;
SamplerState LinearSampler;

struct Face
{
    int idx;
};

[[vk::push_constant]]
ConstantBuffer<Face> FacePS;

struct VS_Output
{
    float4 Position : SV_Position;
    float2 Facecoord : TEXCOORD0;
};

VS_Output VS_Main(uint vertexID : SV_VertexID)
{
    float2 Positions[3] =
    {
        float2(-1.f, -1.f),
        float2(3.f, -1.f),
        float2(-1.f, 3.f)
    };
    VS_Output output;
    output.Position = float4(Positions[vertexID], 0.f, 1.f);
    output.Facecoord = Positions[vertexID];
    
    return output;
}

float4 PS_Main(VS_Output input) : SV_Target0
{  
    
    float3 irradiance = 0.f;
    
    float3 N = GetCubemapDirection(FacePS.idx, input.Facecoord);
    N = normalize(N);
 
    float3 refUp = (abs(N.y) < 0.999f) ? float3(0.f, 1.f, 0.f) : float3(0.f, 0.f, 1.f);
    float3 T = normalize(cross(refUp, N));
    float3 B = normalize(cross(N, T));
    
    float sampleDelta = 0.0015f;
    float nrSamples = 0.0;
   
    for (float phi = 0.f; phi < 2.f * PI; phi += sampleDelta)
    {
        for (float theta = 0.f; theta < 0.5f * PI; theta += sampleDelta)
        {
            float3 tangentSample = float3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta));
            
            float3 sampleDirection = mul(tangentSample, float3x3(T, B, N));
            float3 environmentColor = EnvironmentCubemap.SampleLevel(LinearSampler, sampleDirection, 0.0f).rgb;
            
            irradiance += environmentColor * sin(theta) * cos(theta);
            nrSamples += 1.f;
        }

    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    return float4(irradiance, 1.f);
}




