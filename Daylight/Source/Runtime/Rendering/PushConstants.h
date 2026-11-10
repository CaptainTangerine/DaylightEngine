#pragma once

#include <cstdint>

namespace Dlight
{
	struct FBlurPushConstants
	{
		std::int32_t Direction[2] = {};
	};

	struct FPBRMaterial
	{
		float baseColorFactor[3] = {};
		float metallicFactor = {};
		float roughnessFactor = {};
		float aoFactor = {};
	};

}
