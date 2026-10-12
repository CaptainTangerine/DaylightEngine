#include "pch.h"
#include "Image.h"
#include "stb_image.h"

Dlight::Image::Image(std::string_view filePath)
{
	std::string path(filePath);

	stbi_uc* _pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!_pixels)
	{
		DL_LOG_ERROR("Faild to load texture image");
		abort();
	}

	depth = 1;
	mipLevels = 1;
	format = VK_FORMAT_R8G8B8A8_UNORM;
	pixels.resize(texWidth * texHeight * 4);
	memcpy(pixels.data(), _pixels, pixels.size());
	
	stbi_image_free(_pixels);
}
