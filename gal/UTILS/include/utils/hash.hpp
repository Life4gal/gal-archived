#pragma once

#ifndef GAL_LANG_UTILS_HASH_HPP
#define GAL_LANG_UTILS_HASH_HPP

namespace gal::utils
{
	template<bool Is64Bits = true, typename Container>
		requires requires(Container container)
		{
			std::begin(container);
			std::end(container);
		}
	constexpr std::conditional_t<Is64Bits, std::uint64_t, std::uint32_t> hash(const Container& container) noexcept
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
}

#endif // GAL_LANG_UTILS_HASH_HPP
