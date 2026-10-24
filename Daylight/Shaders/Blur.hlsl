Texture2D<float4> SceneColor;
RWTexture2D<float4> ComputeOutput;

/*
float GaussianWeight(int x, int y, float sigma)
{
    float sigma2 = sigma * sigma;
    float distance2 = float(x * x + y * y);

    return exp(-distance2 / (2.0f * sigma2));
}

float4 BoxBlur(int2 pixel, int radius, int2 maxPos)
{
    float4 sum = 0.0f;
    float sampleCount = 0.0f;
    int2 center = int2(pixel);
    
    for (int x = -radius; x <= radius; ++x)
    {
        for (int y = -radius; y <= radius; ++y)
        {
            int2 samplePos = center + int2(x, y);

            if (any(samplePos < int2(0, 0)) || any(samplePos > maxPos))
            {
                continue;
            }

            sum += SceneColor.Load(int3(samplePos, 0));
            sampleCount += 1.0f;
        }
    }

    return sum / max(sampleCount, 1.0f);
}

float4 GaussianBlur(int2 pixel, int radius, int2 maxPos)
{
    float sigma = radius / 3.f;
    float4 weightedSum = 0.0;
    float weightSum = 0.0;
    int2 center = int2(pixel);

    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            int2 samplePos = center + int2(x, y);

            if (any(samplePos < int2(0, 0)) || any(samplePos > maxPos))
            {
                continue;
            }

            float weight = GaussianWeight(x, y, sigma);
            float4 color = SceneColor.Load(int3(samplePos, 0));

            weightedSum += color * weight;
            weightSum += weight;
        }
    }
    
    return weightedSum / max(weightSum, 0.000001f);
}

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
    
    int2 pixel = dispatchThreadID.xy;
    int2 maxPos = int2(width, height) - 1;
    
    ComputeOutput[pixel] = GaussianBlur(pixel, 20, maxPos);
    
    //ComputeOutput[pixel] = BoxBlur(pixel, 20, maxPos);
}
*/

struct BlurPushConstants
{
    int2 Direction;
};

[[vk::push_constant]]
ConstantBuffer<BlurPushConstants> PushConstants;

float GaussianWeight1D(int offset, float sigma)
{
    float sigma2 = sigma * sigma;
    float distance2 = float(offset * offset);

    return exp(-distance2 / (2.0f * sigma2));
}

float4 GaussianBlur1D(int2 pixel, int radius, int2 maxPos, int2 direction)
{
    float sigma = radius / 3.f;
    float4 weightedSum = 0.0;
    float weightSum = 0.0;

    for (int offset = -radius; offset <= radius; ++offset)
    {
        int2 samplePos = pixel + direction * offset;

        if (any(samplePos < int2(0, 0)) || any(samplePos > maxPos))
        {
            continue;
        }

        float weight = GaussianWeight1D(offset, sigma);
        weightedSum += SceneColor.Load(int3(samplePos, 0)) * weight;
        weightSum += weight;
    }

    return weightedSum / max(weightSum, 0.000001f);
}

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

    int2 pixel = int2(dispatchThreadID.xy);
    int2 maxPos = int2(width, height) - 1;

    ComputeOutput[pixel] = GaussianBlur1D(pixel, 20, maxPos, PushConstants.Direction);
}
