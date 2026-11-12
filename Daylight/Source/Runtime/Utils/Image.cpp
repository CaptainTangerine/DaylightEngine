#include "pch.h"
#include "Image.h"
#include "stb_image.h"

Dlight::Image::Image(std::string_view filePath)
{
	std::string path(filePath);
	depth = 1;
	mipLevels = 1;

	if (stbi_is_hdr(path.c_str()))
	{
		float* loadedPixels = stbi_loadf(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		if (!loadedPixels)
		{
			DL_LOG_WARN("Failed to load HDR texture image: ", path, " (", stbi_failure_reason(), ")");
			return;
		}

		format = VK_FORMAT_R32G32B32A32_SFLOAT;
		pixels.resize(static_cast<size_t>(texWidth) * texHeight * 4 * sizeof(float));
		memcpy(pixels.data(), loadedPixels, pixels.size());

		stbi_image_free(loadedPixels);
		return;
	}

	stbi_uc* loadedPixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!loadedPixels)
	{
		DL_LOG_WARN("Failed to load texture image: ", path, " (", stbi_failure_reason(), ")");
		return;
	}

	format = VK_FORMAT_R8G8B8A8_SRGB;
	pixels.resize(static_cast<size_t>(texWidth) * texHeight * 4);
	memcpy(pixels.data(), loadedPixels, pixels.size());
	
	stbi_image_free(loadedPixels);
}
