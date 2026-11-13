#include "pch.h"
#include "DDSCubemap.h"

#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanImage.h"

#include <fstream>

namespace Dlight
{
	namespace
	{
		constexpr uint32 DDSMagic = 0x20534444;
		constexpr uint32 DDSHeaderFlags = 0x0000100F;
		constexpr uint32 DDSFourCCFlag = 0x00000004;
		constexpr uint32 DDSFourCCDX10 = 0x30315844;
		constexpr uint32 DDSCapsTexture = 0x00001000;
		constexpr uint32 DDSCapsComplex = 0x00000008;
		constexpr uint32 DDSCaps2CubemapAllFaces = 0x0000FE00;
		constexpr uint32 DXGIFormatR16G16B16A16Float = 10;
		constexpr uint32 D3D10ResourceDimensionTexture2D = 3;
		constexpr uint32 D3D11ResourceMiscTextureCube = 0x00000004;
		constexpr VkDeviceSize BytesPerPixel = 8;

		struct FDDSFormat
		{
			uint32 size = 32;
			uint32 flags = DDSFourCCFlag;
			uint32 fourCC = DDSFourCCDX10;
			uint32 rgbBitCount = 0;
			uint32 redMask = 0;
			uint32 greenMask = 0;
			uint32 blueMask = 0;
			uint32 alphaMask = 0;
		};

		struct FDDSHeader
		{
			uint32 size = 124;
			uint32 flags = DDSHeaderFlags;
			uint32 height = 0;
			uint32 width = 0;
			uint32 pitchOrLinearSize = 0;
			uint32 depth = 0;
			uint32 mipMapCount = 1;
			uint32 reserved[11] = {};
			FDDSFormat format{};
			uint32 caps = DDSCapsTexture | DDSCapsComplex;
			uint32 caps2 = DDSCaps2CubemapAllFaces;
			uint32 caps3 = 0;
			uint32 caps4 = 0;
			uint32 reserved2 = 0;
		};

		struct FDDSHeaderDX10
		{
			uint32 dxgiFormat = DXGIFormatR16G16B16A16Float;
			uint32 resourceDimension = D3D10ResourceDimensionTexture2D;
			uint32 miscFlag = D3D11ResourceMiscTextureCube;
			uint32 arraySize = 1;
			uint32 miscFlags2 = 0;
		};

		static_assert(sizeof(FDDSFormat) == 32);
		static_assert(sizeof(FDDSHeader) == 124);
		static_assert(sizeof(FDDSHeaderDX10) == 20);

		VkDeviceSize GetDataSize(uint32 width, uint32 height)
		{
			return static_cast<VkDeviceSize>(width) * height * 6 * BytesPerPixel;
		}
	}

	std::unique_ptr<VulkanImage> DDSCubemap::Load(VulkanDevice& device, const std::filesystem::path& filePath)
	{
		std::ifstream input(filePath, std::ios::binary);
		if (!input)
		{
			return nullptr;
		}

		uint32 magic = 0;
		FDDSHeader header{};
		FDDSHeaderDX10 headerDX10{};
		input.read(reinterpret_cast<char*>(&magic), sizeof(magic));
		input.read(reinterpret_cast<char*>(&header), sizeof(header));
		input.read(reinterpret_cast<char*>(&headerDX10), sizeof(headerDX10));

		const bool validHeader = input &&
			magic == DDSMagic &&
			header.size == sizeof(FDDSHeader) &&
			header.format.size == sizeof(FDDSFormat) &&
			header.format.flags == DDSFourCCFlag &&
			header.format.fourCC == DDSFourCCDX10 &&
			header.width > 0 && header.height > 0 &&
			header.mipMapCount == 1 &&
			(header.caps2 & DDSCaps2CubemapAllFaces) == DDSCaps2CubemapAllFaces &&
			headerDX10.dxgiFormat == DXGIFormatR16G16B16A16Float &&
			headerDX10.resourceDimension == D3D10ResourceDimensionTexture2D &&
			(headerDX10.miscFlag & D3D11ResourceMiscTextureCube) != 0 &&
			headerDX10.arraySize == 1;

		if (!validHeader)
		{
			DL_LOG_ERROR("Unsupported DDS cubemap: ", filePath.string());
			return nullptr;
		}

		const VkDeviceSize dataSize = GetDataSize(header.width, header.height);
		std::vector<uint8> data(static_cast<size_t>(dataSize));
		input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
		if (!input || input.peek() != std::ifstream::traits_type::eof())
		{
			DL_LOG_ERROR("Invalid DDS cubemap data size: ", filePath.string());
			return nullptr;
		}

		FVulkanImageDesc imageDesc{};
		imageDesc.imageType = VK_IMAGE_TYPE_2D;
		imageDesc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		imageDesc.extent = { header.width, header.height, 1 };
		imageDesc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		imageDesc.mipLevels = 1;
		imageDesc.arrayLayers = 6;
		imageDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		imageDesc.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		imageDesc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		auto image = std::make_unique<VulkanImage>(device, imageDesc);
		device.UploadImage(*image, data.data(), dataSize, EImageState::FragmentSampledRead);
		return image;
	}

	bool DDSCubemap::Save(VulkanDevice& device, const VulkanImage& image, const std::filesystem::path& filePath)
	{
		if (image.GetFormat() != VK_FORMAT_R16G16B16A16_SFLOAT ||
			image.GetMipLevels() != 1 || image.GetArrayLayers() != 6 ||
			!(image.GetUsage() & VK_IMAGE_USAGE_TRANSFER_SRC_BIT))
		{
			DL_LOG_ERROR("DDS cubemap save requires a transfer-source RGBA16F cubemap");
			return false;
		}

		const VkDeviceSize dataSize = GetDataSize(image.GetWidth(), image.GetHeight());
		const std::vector<uint8> data = device.DownloadImage(
			image, dataSize, EImageState::FragmentSampledRead, EImageState::FragmentSampledRead);

		std::error_code error;
		std::filesystem::create_directories(filePath.parent_path(), error);
		if (error)
		{
			DL_LOG_ERROR("Failed to create DDS directory: ", error.message());
			return false;
		}

		std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
		if (!output)
		{
			DL_LOG_ERROR("Failed to create DDS cubemap: ", filePath.string());
			return false;
		}

		FDDSHeader header{};
		header.width = image.GetWidth();
		header.height = image.GetHeight();
		header.pitchOrLinearSize = image.GetWidth() * static_cast<uint32>(BytesPerPixel);
		FDDSHeaderDX10 headerDX10{};

		const uint32 magic = DDSMagic;
		output.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
		output.write(reinterpret_cast<const char*>(&header), sizeof(header));
		output.write(reinterpret_cast<const char*>(&headerDX10), sizeof(headerDX10));
		output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

		if (!output)
		{
			DL_LOG_ERROR("Failed to write DDS cubemap: ", filePath.string());
			return false;
		}

		DL_LOG_INFO("Saved DDS cubemap: ", filePath.string());
		return true;
	}
}
