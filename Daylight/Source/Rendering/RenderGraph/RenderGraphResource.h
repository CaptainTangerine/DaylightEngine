#pragma once
#include "Graphics/Vulkan/VulkanImage.h"

namespace Dlight
{
    // 일단 텍스처로 간주

    struct FRenderGraphResource
    {
        VulkanImage image;
        VkImageLayout initialLayout;
        VkImageLayout finalLayout;
    };
}
