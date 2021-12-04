#pragma once

#ifndef GAL_LANG_UTILS_UTILS_HPP
	#define GAL_LANG_UTILS_UTILS_HPP

	#include <cstdint>
	#include <gal.hpp>

/**
 * @brief utility functions.
 */

namespace gal
{
	/**
	 * @brief Returns the number of bytes needed to encode [value] in UTF-8.
	 * Returns 0 if [value] is too large to encode.
	 */
	int			  utf8_encode_num_bytes(int value);

	/**
	 * @brief Encodes value as a series of bytes in [bytes], which is assumed to be large
	 * enough to hold the encoded result.
	 *
	 * Returns the number of written bytes.
	 */
	int			  utf8_encode(int value, std::uint8_t* bytes);

	/**
	 * @brief Decodes the UTF-8 sequence starting at [bytes] (which has max [length]),
	 * returning the code point.
	 *
	 * Returns -1 if the bytes are not a valid UTF-8 sequence.
	 */
	int			  utf8_decode(const std::uint8_t* bytes, gal_size_type length);

	/**
	 * @brief Returns the number of bytes in the UTF-8 sequence starting with [byte].
	 *
	 * If the character at that index is not the beginning of a UTF-8 sequence,
	 * returns 0.
	 */
	int			  utf8_decode_num_bytes(const std::uint8_t byte);

	/**
	 * @brief Returns the smallest power of two that is equal to or greater than [n].
	 */
	gal_size_type bit_ceil(gal_size_type n);

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
