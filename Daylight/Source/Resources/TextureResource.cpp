#include "pch.h"
#include "TextureResource.h"
#include "Graphics/Vulkan/VulkanImage.h"
#include "Graphics/Vulkan/VulkanSampler.h"

namespace Dlight
{
    TextureResource::TextureResource(const std::string& id, VulkanDevice& _device, FTextureRaw _textureRaw)
        :Resource(id), device(_device), textureRaw(_textureRaw)
    {
    }

    TextureResource::~TextureResource()
    {
    }

    bool TextureResource::DoLoad()
    {
        if (IsLoaded())
        {
            return true;
        }

        image = CreateImage2D(textureRaw.image);
        
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = textureRaw.sampler.magFilter;
        samplerInfo.minFilter = textureRaw.sampler.minFilter;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = textureRaw.sampler.addressU;
        samplerInfo.addressModeV = textureRaw.sampler.addressV;
        samplerInfo.addressModeW = textureRaw.sampler.addressW;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        sampler = std::make_unique<VulkanSampler>(device, samplerInfo);

        return true;
    }

    std::unique_ptr<VulkanImage> TextureResource::CreateImage2D(Image& image)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = image.GetFormat();
        imageInfo.extent = {
            static_cast<uint32>(image.GetWidth()),
            static_cast<uint32>(image.GetHeight()),
            1
        };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = image.GetFormat();
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.layerCount = 1;

        auto gpuImage = std::make_unique<VulkanImage>(device, imageInfo, imageViewInfo);

        device.UploadImage(*gpuImage, image.GetData(), image.GetSize(), EResourceState::Undefined, EResourceState::FragmentShaderRead);

        return std::move(gpuImage);
    }

    void TextureResource::DoUnload()
    {
        image.reset();
        sampler.reset();
    }
}
