#pragma once

#ifndef GAL_LANG_UTILS_UTILS_HPP
	#define GAL_LANG_UTILS_UTILS_HPP

	#include <cstdint>
	#include <limits>
	#include <utility>
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
	int			  utf8_decode_num_bytes(std::uint8_t byte);

	/**
	 * @brief Returns the smallest power of two that is equal to or greater than [n].
	 */
	gal_size_type bit_ceil(gal_size_type n);

	/**
	 * @brief get the real index if `size_type` and `index_type` has different type
	 *
	 * Bounded:
	 *      [1,    2,    3,    4,    5]
	 *       ^0    ^1    ^2    ^3    ^4
	 *       ^-5   ^-4   ^-3   ^-2   ^-1
	 * Unbounded:
	 *      [1,    2,    3,    4,    5]    [insertable position here]
	 *       ^0    ^1    ^2    ^3    ^4    ^5
	 *       ^-6   ^-5   ^-4   ^-3   ^-2   ^-1
	 */
	template<bool Bounded = true>
	[[nodiscard]] constexpr auto index_to_size(std::integral auto target_size, std::integral auto index) noexcept -> decltype(target_size)
	{
		using size_type = decltype(target_size);

		auto ret = static_cast<size_type>(index);
		if (std::cmp_greater(ret, index))
		{
			// negative index
			if constexpr (Bounded)
			{
				ret = target_size - (std::numeric_limits<size_type>::max() - ret + 1);
			}
			else
			{
				ret = target_size - (std::numeric_limits<size_type>::max() - ret + 1) + 1;
			}
		}
		return ret;
	}
}// namespace gal

#endif//GAL_LANG_UTILS_UTILS_HPP
