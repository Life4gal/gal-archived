#pragma once

#ifndef GAL_UTILS_HASH_HPP
#define GAL_UTILS_HASH_HPP

#include <utility>

namespace gal::utils
{
	template<bool Is64Bits = true, typename Container>
		requires requires(Container container)
		{
			std::begin(container);
			std::end(container);
		}
	[[nodiscard]] constexpr std::conditional_t<Is64Bits, std::uint64_t, std::uint32_t> fnv1a_hash(const Container& container) noexcept
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

	template<typename Container>
	requires requires(Container container)
	{
		std::begin(container);
		std::end(container);
		std::size(container);
		std::data(container);
	}
	[[nodiscard]] constexpr std::size_t short_string_hash(const Container& container) noexcept
	{
		const auto* data = std::data(container);
		const auto	length = std::size(container);

		std::size_t hash   = static_cast<std::size_t>(length);

		for (auto i = length; i > 0; --i)
		{
			hash ^= (hash << 5) + (hash >> 2) + data[i - 1];
		}

		return hash;
	}
}

#endif // GAL_UTILS_HASH_HPP
