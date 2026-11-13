#include "pch.h"
#include "TextureResource.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanImage.h"
#include "Graphics/Vulkan/VulkanSampler.h"
#include "Utils/Image.h"

namespace Dlight
{
	TextureResource::TextureResource(const std::string& id, VulkanDevice& _device, std::filesystem::path _imagePath, ETextureSemantic _semantic, FSamplerDesc _samplerDesc)
		: Resource(id)
		, device(_device)
		, imagePath(std::move(_imagePath))
		, semantic(_semantic)
		, samplerDesc(_samplerDesc)
	{
	}

	TextureResource::~TextureResource() = default;

	bool TextureResource::DoLoad()
	{
		if (IsLoaded())
		{
			return true;
		}

		std::error_code error;
		if (std::filesystem::is_regular_file(imagePath, error))
		{
			Image sourceImage(imagePath.string());
			if (sourceImage.IsValid())
			{
				image = CreateImage2D(sourceImage);
			}
		}

		if (!image && !imagePath.empty())
		{
			DL_LOG_WARN("Using default texture for resource: ", GetID(), " (", imagePath.string(), ")");
		}

		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = samplerDesc.magFilter;
		samplerInfo.minFilter = samplerDesc.minFilter;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = samplerDesc.addressU;
		samplerInfo.addressModeV = samplerDesc.addressV;
		samplerInfo.addressModeW = samplerDesc.addressW;
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

	const VulkanImage* TextureResource::GetImage() const { return image ? image.get() : &VulkanImage::GetDefaultImage(GetDefaultImageType()); }

	std::unique_ptr<VulkanImage> TextureResource::CreateImage2D(Image& image)
	{
		FVulkanImageDesc imageDesc{};
		imageDesc.format = GetImageFormat(image);
		imageDesc.extent = { static_cast<uint32>(image.GetWidth()), static_cast<uint32>(image.GetHeight()), 1 };
		imageDesc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		auto gpuImage = std::make_unique<VulkanImage>(device, imageDesc);

		device.UploadImage(*gpuImage, image.GetData(), image.GetSize(), EImageState::FragmentSampledRead);

		return gpuImage;
	}

	VkFormat TextureResource::GetImageFormat(const Image& sourceImage) const
	{
		if (sourceImage.GetFormat() != VK_FORMAT_R8G8B8A8_SRGB)
		{
			return sourceImage.GetFormat();
		}

		return semantic == ETextureSemantic::BaseColor ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	}

	EDefaultImage TextureResource::GetDefaultImageType() const
	{
		switch (semantic)
		{
		case ETextureSemantic::Normal:
		{
			return EDefaultImage::FlatNormal;
		}
		case ETextureSemantic::Height:
		{
			return EDefaultImage::Black;
		}
		case ETextureSemantic::BaseColor:
		case ETextureSemantic::Metallic:
		case ETextureSemantic::Roughness:
		case ETextureSemantic::AmbientOcclusion:
		{
			return EDefaultImage::White;
		}
		}

		return EDefaultImage::White;
	}

	void TextureResource::DoUnload()
	{
		image.reset();
		sampler.reset();
	}
}
