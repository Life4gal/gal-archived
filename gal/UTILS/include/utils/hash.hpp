#pragma once

#ifndef GAL_UTILS_HASH_HPP
#define GAL_UTILS_HASH_HPP

#include <cstdint>

namespace gal::utils
{
	template<bool Is64Bits = true>
	[[nodiscard]] constexpr std::conditional_t<Is64Bits, std::uint64_t, std::uint32_t> hash_fnv1a(const std::ranges::range auto& container) noexcept
	{
		// FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
		if constexpr (Is64Bits)
		{
			constexpr std::uint64_t hash_init{14695981039346656037ull};
			constexpr std::uint64_t hash_prime{1099511628211ull};

			auto hash = hash_init;

			for (const auto i: container)
			{
				hash ^= i;
				hash *= hash_prime;
			}

			return hash;
		}
		else
		{
			constexpr std::uint32_t hash_init{2166136261u};
			constexpr std::uint32_t hash_prime{16777619u};

			auto hash = hash_init;

			for (const auto i: container)
			{
				hash ^= i;
				hash *= hash_prime;
			}

			return hash;
		}
	}

	[[nodiscard]] constexpr std::uint32_t hash_jenkins_one_at_a_time(const std::ranges::range auto& container) noexcept
	{
		std::uint32_t hash = 0;

		for (const auto i: container)
		{
			hash += static_cast<std::uint32_t>(i);
			hash += hash << 10;
			hash ^= hash >> 6;
		}

		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;
		return hash;
	}
}

#endif // GAL_UTILS_HASH_HPP
