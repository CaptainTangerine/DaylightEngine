#pragma once

#include <filesystem>
#include <memory>

namespace Dlight
{
	class VulkanDevice;
	class VulkanImage;

	class DDSCubemap
	{
	public:
		static std::unique_ptr<VulkanImage> Load(VulkanDevice& device, const std::filesystem::path& filePath);
		static bool Save(VulkanDevice& device, const VulkanImage& image, const std::filesystem::path& filePath);
	};
}
