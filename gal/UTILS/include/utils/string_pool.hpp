#pragma once

#ifndef GAL_LANG_UTILS_STRING_POOL_HPP
#define GAL_LANG_UTILS_STRING_POOL_HPP

#include <algorithm>
#include <memory>
#include <utils/assert.hpp>
#include <vector>

namespace gal::utils
{
	template<typename CharType, bool IsNullTerminate = true, typename CharTrait = std::char_traits<CharType>>
	class string_block
	{
	public:
		constexpr static bool is_null_terminate = IsNullTerminate;

		using view_type = std::basic_string_view<CharType, CharTrait>;
		using value_type = typename view_type::value_type;
		using size_type = typename view_type::size_type;

		constexpr explicit string_block(size_type capacity)
			: memory_(new value_type[capacity]),
			  capacity_(capacity),
			  size_(0) {}

		[[nodiscard]] constexpr static size_type length_of(view_type str) noexcept
		{
			if constexpr (is_null_terminate) { return str.length() + 1; }
			else { return str.length(); }
		}

		[[nodiscard]] constexpr view_type append(view_type str)
		{
			// todo: shall we re-alloc more memory?
			gal_assert(storable(str), "There is not enough space for this string.");

			const auto dest = memory_.get() + size_;
			std::ranges::copy(str, dest);

			if constexpr (is_null_terminate) { dest[str.length()] = 0; }

			size_ += length_of(str);
			return {dest, str.length()};
		}

		[[nodiscard]] constexpr bool storable(view_type str) const noexcept { return available_space() >= length_of(str); }

		[[nodiscard]] constexpr size_type available_space() const noexcept { return capacity_ - size_; }

		[[nodiscard]] constexpr bool more_available_space_than(const string_block& other) { return available_space() > other.available_space(); }

	private:
		std::unique_ptr<value_type[]> memory_;
		size_type capacity_;
		size_type size_;
	};

	template<typename CharType, bool IsNullTerminate = true, typename CharTrait = std::char_traits<CharType>>
	class string_pool
	{
	public:
		using block_type = string_block<CharType, IsNullTerminate, CharTrait>;
		using pool_type = std::vector<block_type>;

		using view_type = typename block_type::view_type;
		using value_type = typename block_type::value_type;
		using size_type = typename block_type::size_type;

		constexpr string_pool() = default;

		constexpr explicit string_pool(size_type capacity)
			: capacity_(capacity) {}

		[[nodiscard]] constexpr view_type append(view_type str) { return append_str_into_block(str, find_or_create_block(str)); }

		[[nodiscard]] constexpr size_type size() const noexcept { return pool_.size(); }

		[[nodiscard]] constexpr size_type capacity() const noexcept { return capacity_; }


		// Only affect the block created after modification
		constexpr void new_capacity(size_type capacity) noexcept { capacity_ = capacity; }

	private:
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

		pool_type pool_;
		const size_type capacity_ = 8196;
	};
}

#endif // GAL_LANG_UTILS_STRING_POOL_HPP
