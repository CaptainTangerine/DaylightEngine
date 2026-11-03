#pragma once

#include "vulkan/vulkan.h"

namespace Dlight
{
	class Image
	{
	public:
		Image(std::string_view filePath);
		~Image() = default;

	public:
		int32 GetWidth() const { return texWidth; };
		int32 GetHeight() const { return texHeight; };
		int32 GetDepth() const { return depth; };
		int32 GetMipLevel() const { return mipLevels; };
		VkFormat GetFormat() const { return format; };

		void* GetData() { return pixels.data(); };
		uint64 GetSize() const { return pixels.size(); };

	private:
		int32 texWidth = {};
		int32 texHeight = {};
		int32 texChannels = {};
		int32 depth = {};
		int32 mipLevels = {};

		VkFormat format = {};

		std::vector<uint8> pixels;
	};
}
