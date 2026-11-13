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

		bool CreateIrradianceMap();
		const VulkanImage* GetIrradianceMap() const { return irradianceMap.get(); }

	private:
		// EnvironmentCubeMap
		void CreateCubemapSampler();
		void CreateCubemapDescriptorSetLayout();
		void CreateCubemapDescriptorPool();
		void AllocateCubemapDescriptorSet();
		void WriteCubemapDescriptorSet(const VulkanImage& sourceTexture);
		bool CreateCubemapGraphicsPipeline(VkDescriptorSetLayout descriptorSetLayout);
		void RenderCube();
		void DestroyCubemapResources();

		// Irradiance Map
		void CreateIrradianceMapSampler();
		void CreateIrradianceMapDescriptorSetLayout();
		void CreateIrradianceMapDescriptorPool();
		void AllocateIrradianceMapDescriptorSet();
		void WriteIrradianceMapDescriptorSet(const VulkanImage& sourceCubemap);
		bool CreateIrradianceMapGraphicsPipeline(VkDescriptorSetLayout descriptorSetLayout);
		void RenderIrradianceMap();
		void DestroyIrradianceMapResources();

	private:
		VulkanDevice& device;
		ShaderCompiler& shaderCompiler;

		uint32 faceSize = {};
		uint32 irradianceMapFaceSize = 32;

		// cubemap
		std::unique_ptr<ShaderResource> cubemapVertexShader;
		std::unique_ptr<ShaderResource> cubemapFragmentShader;

		VkDescriptorSetLayout cubemapDescriptorSetLayout = { VK_NULL_HANDLE };
		VkDescriptorPool cubemapDescriptorPool = { VK_NULL_HANDLE };
		VkDescriptorSet cubemapDescriptorSet = { VK_NULL_HANDLE };

		std::unique_ptr<VulkanGraphicsPipeline> cubemapPipeline;
		std::unique_ptr<VulkanSampler> cubemapSampler;

		// irradianceMap
		std::unique_ptr<ShaderResource> irradianceMapVertexShader;
		std::unique_ptr<ShaderResource> irradianceMapFragmentShader;

		VkDescriptorSetLayout irradianceMapDescriptorSetLayout = { VK_NULL_HANDLE };
		VkDescriptorPool irradianceMapDescriptorPool = { VK_NULL_HANDLE };
		VkDescriptorSet irradianceMapDescriptorSet = { VK_NULL_HANDLE };

		std::unique_ptr<VulkanGraphicsPipeline> irradianceMapPipeline;
		std::unique_ptr<VulkanSampler> irradianceMapSampler;

		std::unique_ptr<VulkanImage> cubemap;
		std::unique_ptr<VulkanImage> irradianceMap;
	};
}
