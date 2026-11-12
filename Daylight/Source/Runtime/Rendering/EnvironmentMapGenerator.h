#pragma once

#include <array>

#include "Graphics/Vulkan/VulkanDevice.h"

namespace Dlight
{
	class Image;
	class ShaderCompiler;
	class ShaderResource;
	class VulkanDevice;
	class VulkanGraphicsPipeline;
	class VulkanImage;
	class VulkanSampler;

	constexpr VkFormat CubemapFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

	class EnvironmentMapGenerator
	{
	public:
		EnvironmentMapGenerator(VulkanDevice& device, ShaderCompiler& shaderCompiler);
		~EnvironmentMapGenerator();

		EnvironmentMapGenerator(const EnvironmentMapGenerator&) = delete;
		EnvironmentMapGenerator& operator=(const EnvironmentMapGenerator&) = delete;

		bool CreateCubemap(const Image& srcImage);
		const VulkanImage* GetCubemap() const { return cubemap.get(); }

	private:
		void CreateConversionSampler();
		bool CreateConversionDescriptorSetLayout();
		bool CreateConversionDescriptorPool();
		bool AllocateConversionDescriptorSet();
		void WriteConversionDescriptorSet(const VulkanImage& sourceTexture);
		bool CreateConversionGraphicsPipeline(VkDescriptorSetLayout descriptorSetLayout);
		void ExecuteCubemapConversion(const std::array<VkImageView, 6>& faceViews, uint32 faceSize);
		void DestroyConversionResources();

	private:
		VulkanDevice& device;
		ShaderCompiler& shaderCompiler;

		std::unique_ptr<ShaderResource> conversionVertexShader;
		std::unique_ptr<ShaderResource> conversionFragmentShader;

		VkDescriptorSetLayout conversionDescriptorSetLayout = { VK_NULL_HANDLE };
		VkDescriptorPool conversionDescriptorPool = { VK_NULL_HANDLE };
		VkDescriptorSet conversionDescriptorSet = { VK_NULL_HANDLE };

		std::unique_ptr<VulkanGraphicsPipeline> conversionPipeline;
		std::unique_ptr<VulkanSampler> conversionSampler;

		std::unique_ptr<VulkanImage> cubemap;
	};
}
