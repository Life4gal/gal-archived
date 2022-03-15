#pragma once

#ifndef GAL_UTILS_STRING_POOL_HPP
#define GAL_UTILS_STRING_POOL_HPP

#include <algorithm>
#include <memory>
#include <utils/assert.hpp>
#include <vector>

namespace gal::utils
{
	template<typename CharType, bool IsNullTerminate = true, typename CharTrait = std::char_traits<CharType>>
	class string_pool
	{
		template<typename BlockCharType, bool BlockIsNullTerminate, typename BlockCharTrait>
		class string_block
		{
			template<typename, bool, typename>
			friend class string_block;
		public:
			constexpr static bool is_null_terminate = IsNullTerminate;

			using view_type = std::basic_string_view<CharType, CharTrait>;
			using value_type = typename view_type::value_type;
			using size_type = typename view_type::size_type;

			constexpr static CharType invalid_char{'\0'};

		private:
			std::unique_ptr<value_type[]> memory_;
			size_type capacity_;
			size_type size_;

		public:
			constexpr explicit string_block(size_type capacity)
				: memory_{std::make_unique<value_type[]>(capacity)},
				  capacity_{capacity},
				  size_{0} {}

			[[nodiscard]] constexpr static size_type length_of(view_type str) noexcept
			{
				if constexpr (is_null_terminate) { return str.length() + 1; }
				else { return str.length(); }
			}

			[[nodiscard]] constexpr view_type append(view_type str)
			{
				if (not storable(str))
				{
					gal_assert(storable(str), "There are not enough space for this string.");
					return &invalid_char;
				}

				const auto dest = memory_.get() + size_;
				std::ranges::copy(str, dest);

				if constexpr (is_null_terminate) { dest[str.length()] = 0; }

				size_ += length_of(str);
				return {dest, str.length()};
			}

			[[nodiscard]] constexpr bool storable(view_type str) const noexcept { return available_space() >= length_of(str); }

			[[nodiscard]] constexpr size_type available_space() const noexcept { return capacity_ - size_; }

			[[nodiscard]] constexpr bool more_available_space_than(const string_block& other) { return available_space() > other.available_space(); }

			friend constexpr void swap(string_block& lhs, string_block& rhs) noexcept
			{
				using std::swap;
				swap(lhs.memory_, rhs.memory_);
				swap(lhs.capacity_, rhs.capacity_);
				swap(lhs.size_, rhs.size_);
			}
		};

		using block_type = string_block<CharType, IsNullTerminate, CharTrait>;
		using pool_type = std::vector<block_type>;

	public:
		using view_type = typename block_type::view_type;
		using value_type = typename block_type::value_type;
		using size_type = typename block_type::size_type;

	private:
		pool_type pool_;
		size_type capacity_;

		using block_iterator = typename pool_type::iterator;

		[[nodiscard]] constexpr view_type append_str_into_block(view_type str, block_iterator pos)
		{
			const auto ret = pos->append(str);
			shake_it(pos);
			return ret;
		}

		[[nodiscard]] constexpr block_iterator find_or_create_block(view_type str)
		{
			if (const auto block = find_storable_block(str); block != pool_.end()) { return block; }
			return create_storable_block(str);
		}

		[[nodiscard]] constexpr block_iterator find_first_possible_storable_block(view_type str) noexcept
		{
			if (pool_.size() > 2 && not std::ranges::prev(pool_.end(), 2)->storable(str)) { return std::ranges::prev(pool_.end()); }
			return pool_.begin();
		}

		[[nodiscard]] constexpr block_iterator find_storable_block(view_type str) noexcept
		{
			return std::ranges::lower_bound(
					find_first_possible_storable_block(str),
					pool_.end(),
					true,
					[](bool b, bool) { return b; },
					[&str](const auto& block) { return not block.storable(str); });
		}

		[[nodiscard]] constexpr block_iterator create_storable_block(view_type str)
		{
			pool_.emplace_back(std::ranges::max(capacity_, block_type::length_of(str)));
			return std::ranges::prev(pool_.end());
		}

		constexpr void shake_it(block_iterator block)
		{
			if (
				block == pool_.begin() ||
				block->more_available_space_than(*std::ranges::prev(block))) { return; }

			if (const auto it =
						std::ranges::upper_bound(
								pool_.begin(),
								block,
								block->available_space(),
								std::ranges::less{},
								[](const auto& b) { return b.available_space(); });
				it != block) { std::ranges::rotate(it, block, std::ranges::next(block)); }
		}

	public:
		constexpr explicit string_pool(size_type capacity = 8196) noexcept(std::is_nothrow_default_constructible_v<pool_type>)
			: capacity_(capacity) {}

		template<std::same_as<string_pool>... Pools>
		constexpr explicit string_pool(Pools&&... pools)
		{
			pool_.reserve((pools.pool_.size() + ...));

			block_iterator iterator;
			(((iterator = pool_.insert(pool_.end(), std::make_move_iterator(pools.pool_.begin()), std::make_move_iterator(pools.pool_.end()))),
			  pools.pool_.clear(),
			  std::ranges::inplace_merge(pool_.begin(), iterator, pool_.end(), [](const auto& a, const auto& b) { return not a.more_available_space_than(b); })),
				...);
		}

		[[nodiscard]] constexpr view_type append(view_type str) { return append_str_into_block(str, find_or_create_block(str)); }

		[[nodiscard]] constexpr size_type size() const noexcept { return pool_.size(); }

		[[nodiscard]] constexpr size_type capacity() const noexcept { return capacity_; }

		// Only affect the block created after modification
		constexpr void resize(size_type capacity) noexcept { capacity_ = capacity; }
	};
}

#endif // GAL_UTILS_STRING_POOL_HPP
