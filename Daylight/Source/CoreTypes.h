#pragma once

#include <cstddef>
#include <cstdint>

namespace Dlight
{
	using int8   = std::int8_t;
	using int16  = std::int16_t;
	using int32  = std::int32_t;
	using int64  = std::int64_t;

	using uint8  = std::uint8_t;
	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;
	using uint64 = std::uint64_t;

	static_assert(sizeof(int8)   == 1, "int8 must be 1 byte");
	static_assert(sizeof(int16)  == 2, "int16 must be 2 bytes");
	static_assert(sizeof(int32)  == 4, "int32 must be 4 bytes");
	static_assert(sizeof(int64)  == 8, "int64 must be 8 bytes");

	static_assert(sizeof(uint8)  == 1, "uint8 must be 1 byte");
	static_assert(sizeof(uint16) == 2, "uint16 must be 2 bytes");
	static_assert(sizeof(uint32) == 4, "uint32 must be 4 bytes");
	static_assert(sizeof(uint64) == 8, "uint64 must be 8 bytes");
}
