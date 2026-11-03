#pragma once
#include "Resource.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Utils/Image.h"

namespace Dlight
{
    class VulkanImage;
    class VulkanSampler;

    struct FSamplerDesc
    {
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkFilter magFilter = VK_FILTER_LINEAR;

        VkSamplerAddressMode addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    };

    struct FTextureRaw
    {
        Image        image;
        FSamplerDesc sampler;
    };

    class TextureResource : public Resource
    {
    public:
        TextureResource(const std::string& id, VulkanDevice& _device, FTextureRaw _textureRaw);
        virtual ~TextureResource();

        const VulkanImage* GetImage() const { return image.get(); }
        const VulkanSampler* GetSampler() const { return sampler.get(); }

    protected:
        virtual bool DoLoad();
        virtual void DoUnload();

    private:
        std::unique_ptr<VulkanImage> CreateImage2D(Image& image);

    private:
        VulkanDevice& device;
        FTextureRaw textureRaw;

        std::unique_ptr<VulkanImage> image = {};
        std::unique_ptr<VulkanSampler> sampler = {};
    };
}
