#pragma once

#include <cstdint>

namespace Dlight
{
	struct FBlurPushConstants
	{
		std::int32_t Direction[2] = {};
	};

	static_assert(sizeof(FBlurPushConstants) == 8, "Blur push constants must match HLSL int2 (8 bytes)");
}
