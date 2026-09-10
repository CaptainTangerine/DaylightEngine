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

	// 크기 / 인덱스 / 오프셋. 포인터 폭을 따라간다.
	using usize  = std::size_t;
	using isize  = std::ptrdiff_t;

	// 플랫폼이 바뀌어도 위 가정이 깨지지 않도록 컴파일 타임에 못박는다.
	// (unsigned long 은 Windows 4바이트 / Linux 8바이트로 갈리므로 직접 쓰지 않는다.)
	static_assert(sizeof(int8)   == 1, "int8 must be 1 byte");
	static_assert(sizeof(int16)  == 2, "int16 must be 2 bytes");
	static_assert(sizeof(int32)  == 4, "int32 must be 4 bytes");
	static_assert(sizeof(int64)  == 8, "int64 must be 8 bytes");

	static_assert(sizeof(uint8)  == 1, "uint8 must be 1 byte");
	static_assert(sizeof(uint16) == 2, "uint16 must be 2 bytes");
	static_assert(sizeof(uint32) == 4, "uint32 must be 4 bytes");
	static_assert(sizeof(uint64) == 8, "uint64 must be 8 bytes");

	static_assert(sizeof(usize)  == sizeof(void*), "usize must match pointer width");
	static_assert(sizeof(isize)  == sizeof(void*), "isize must match pointer width");
}
