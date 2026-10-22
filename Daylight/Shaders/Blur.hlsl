Texture2D<float4> SceneColor;
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> ComputeOutput;

[numthreads(8,8,1)]
void CS_Main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    ComputeOutput.GetDimensions(width, height);
    if (width <= dispatchThreadID.x || height <= dispatchThreadID.y)
    {
        return;
    }
    
    uint2 pixel = dispatchThreadID.xy;
    float4 color = SceneColor.Load(int3(pixel, 0));
    ComputeOutput[pixel] = color;
}
