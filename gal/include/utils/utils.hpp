#pragma once

#ifndef GAL_LANG_UTILS_UTILS_HPP
	#define GAL_LANG_UTILS_UTILS_HPP

	#include <cstdint>

namespace gal
{
	/**
	 * @brief A union to let us reinterpret a double as raw bits and back.
	 */
	union double_bits
	{
		std::uint64_t bits64;
		std::uint32_t bits32[2];
		double		  number;
	};

	constexpr std::uint64_t double_qnan_pos_min_bits{0x7FF8000000000000};
	constexpr std::uint64_t double_qnan_pos_max_bits{0x7FFFFFFFFFFFFFFF};

	constexpr double		bits_to_double(std::uint64_t bits)
	{
		return double_bits{.bits64 = bits}.number;
	}

	constexpr std::uint64_t double_to_bits(double number)
	{
		return double_bits{.number = number}.bits64;
	}
}// namespace gal

#endif//GAL_LANG_UTILS_UTILS_HPP
