#pragma once
#include "Resource.h"

#include <filesystem>
#include <vulkan/vulkan.h>

namespace Dlight
{
    class Image;
    class VulkanDevice;
    enum class EDefaultImage;
    class VulkanImage;
    class VulkanSampler;

    enum class ETextureSemantic
    {
        BaseColor,
        Normal,
        Height,
        Metallic,
        Roughness,
        AmbientOcclusion
    };

    struct FSamplerDesc
    {
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkFilter magFilter = VK_FILTER_LINEAR;

        VkSamplerAddressMode addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    };

    class TextureResource : public Resource
    {
    public:
        TextureResource(const std::string& id, VulkanDevice& device, std::filesystem::path imagePath, ETextureSemantic semantic = ETextureSemantic::BaseColor, FSamplerDesc samplerDesc = {});
        virtual ~TextureResource();

        const VulkanImage* GetImage() const;
        const VulkanSampler* GetSampler() const { return sampler.get(); }

    protected:
        virtual bool DoLoad();
        virtual void DoUnload();

    private:
        std::unique_ptr<VulkanImage> CreateImage2D(Image& image);
        VkFormat GetImageFormat(const Image& sourceImage) const;
        EDefaultImage GetDefaultImageType() const;

    private:
        VulkanDevice& device;
        std::filesystem::path imagePath;
        ETextureSemantic semantic;
        FSamplerDesc samplerDesc;

        std::unique_ptr<VulkanImage> image = {};
        std::unique_ptr<VulkanSampler> sampler = {};
    };
}
