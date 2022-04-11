#pragma once

#ifndef GAL_UTILS_STRING_POOL_HPP
#define GAL_UTILS_STRING_POOL_HPP

#include <algorithm>
#include <memory>
#include <ranges>
#include <utils/assert.hpp>
#include <vector>

namespace gal::utils
{
	template<typename CharType = char, bool IsNullTerminate = true, typename CharTrait = std::char_traits<CharType>>
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
			constexpr explicit string_block(const size_type capacity)
				: memory_{std::make_unique<value_type[]>(capacity)},
				  capacity_{capacity},
				  size_{0} {}

			[[nodiscard]] constexpr static size_type length_of(const view_type str) noexcept
			{
				if constexpr (is_null_terminate) { return str.length() + 1; }
				else { return str.length(); }
			}

			[[nodiscard]] constexpr view_type append(const view_type str) noexcept
			{
				if (not this->storable(str))
				{
					gal_assert(this->storable(str), "There are not enough space for this string.");
					return &invalid_char;
				}

				const auto dest = memory_.get() + size_;
				std::ranges::copy(str, dest);

				if constexpr (is_null_terminate) { dest[str.length()] = 0; }

				size_ += this->length_of(str);
				return {dest, str.length()};
			}

			[[nodiscard]] constexpr value_type* borrow_raw(const size_type size) noexcept
			{
				if (not this->storable(size))
				{
					gal_assert(this->storable(size), "There are not enough space for this string.");
					return nullptr;
				}

				const auto dest = memory_.get() + size_;

				if constexpr (is_null_terminate) { dest[size] = 0; }
				size_ += size;

				return dest;
			}

			constexpr void return_raw(const value_type* raw, const size_type size) noexcept
			{
				auto memory_data = memory_.get();

				gal_assert(memory_data >= (raw - (size_ - size)), "The given memory does not belong to this block");

				if (memory_data > (raw - (size_ - size)))
				{
					// In this case, new string is inserted after the memory is borrowed, and the newly added string needs to be moved to the front
					auto move_dest = memory_data + (raw - memory_data);// raw;
					gal_assert(move_dest == raw);
					auto move_source = raw + size;
					auto move_size = size_ - size - (raw - memory_data);

					std::ranges::copy_n(move_source, move_size, move_dest);
				}

				size_ -= size;

				if constexpr (is_null_terminate) { memory_.get()[size_] = 0; }
			}

			[[nodiscard]] constexpr bool storable(const view_type str) const noexcept { return available_space() >= this->length_of(str); }

			[[nodiscard]] constexpr bool storable(const size_type size) const noexcept { return available_space() >= size; }

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

	public:
		// todo: The borrower has many optimizations,
		// such as separating the borrow pool from the actual pool,
		// inserting strings into the borrower pool first,
		// and then inserting it back into the actual pool when the borrower is destructed.
		// The current implementation is to record where all strings are inserted, and then delete them one by one, which involves multiple moves (to move the following strings forward)
		class block_borrower
		{
			std::reference_wrapper<string_pool> pool_;
			// pair -> block memory view <=> which block
			std::vector<std::pair<view_type, block_iterator>> borrowed_blocks_;
			bool need_return_ = true;

		public:
			[[nodiscard]] constexpr bool need_return() const noexcept { return need_return_; }

			constexpr void break_promise() noexcept { need_return_ = false; }

			explicit constexpr block_borrower(string_pool& pool) noexcept
				: pool_{pool} {}

			constexpr block_borrower(const block_borrower&) = delete;
			constexpr block_borrower& operator=(const block_borrower&) = delete;
			constexpr block_borrower(block_borrower&&) noexcept = default;
			constexpr block_borrower& operator=(block_borrower&&) = default;

			constexpr ~block_borrower() noexcept
			{
				if (need_return_)
				{
					std::ranges::for_each(
							// back to front, because the last inserted string is saved at the end
							borrowed_blocks_ | std::views::reverse,
							[this](auto& pair) mutable
							{
								auto& pool = pool_.get();

								pool.return_raw_memory(pair.first, pair.second);
								// todo: maybe a lock is needed here?
								// erase is not performed because the target may still have memory in use.
								// pool.erase(pair.second);
							});
				}
			}

			/**
			 * @brief Add a string to the pool, and then you can freely use the added string.
			 */
			constexpr view_type append(const view_type str)
			{
				// see also: string_pool::append

				// not only to insert the string, but also to save the inserted position
				auto& pool = pool_.get();

				auto pos = pool.find_or_create_block(str);
				return borrowed_blocks_.emplace_back(pool.append_str_into_block(str, pos), pos).first;
			}

			/**
			 * @brief Borrow a block of memory to the pool, users can directly write strings in this memory area without worrying about its invalidation.
			 */
			[[nodiscard]] constexpr value_type* borrow_raw(const size_type size)
			{
				// see also: string_pool::borrow_raw

				// not only to insert the string, but also to save the inserted position
				auto& pool = pool_.get();

				auto pos = pool.find_or_create_block(size);
				return borrowed_blocks_.emplace_back({pool.borrow_raw_memory(size, pos), size}, pos).first.data();
			}
		};

	private:
		[[nodiscard]] constexpr view_type append_str_into_block(const view_type str, block_iterator pos)
		{
			const auto ret = pos->append(str);
			this->shake_it(pos);
			return ret;
		}

		[[nodiscard]] constexpr value_type* borrow_raw_memory(const size_type size, block_iterator pos)
		{
			auto raw = pos->borrow_raw(size);
			this->shake_it(pos);
			return raw;
		}

		constexpr void return_raw_memory(const value_type* raw, const size_type size, block_iterator pos)
		{
			pos->return_raw(raw, size);
			this->shake_it(pos);
		}

		constexpr void return_raw_memory(view_type view, block_iterator pos) { this->return_raw_memory(view.data(), view.size(), pos); }

		[[nodiscard]] constexpr block_iterator find_or_create_block(const size_type size)
		{
			if (const auto block = this->find_storable_block(size); block != pool_.end()) { return block; }
			return this->create_storable_block(size);
		}

		[[nodiscard]] constexpr block_iterator find_or_create_block(const view_type str) { return this->find_or_create_block(str.size()); }

		[[nodiscard]] constexpr block_iterator find_first_possible_storable_block(const size_type size) noexcept
		{
			if (pool_.size() > 2 && not std::ranges::prev(pool_.end(), 2)->storable(size)) { return std::ranges::prev(pool_.end()); }
			return pool_.begin();
		}

		[[nodiscard]] constexpr block_iterator find_first_possible_storable_block(const view_type str) noexcept { return this->find_first_possible_storable_block(str.size()); }

		[[nodiscard]] constexpr block_iterator find_storable_block(const size_type size) noexcept
		{
			return std::ranges::lower_bound(
					this->find_first_possible_storable_block(size),
					pool_.end(),
					true,
					[](bool b, bool) { return b; },
					[size](const auto& block) { return not block.storable(size); });
		}

		[[nodiscard]] constexpr block_iterator find_storable_block(const view_type str) noexcept { return this->find_storable_block(str.size()); }

		[[nodiscard]] constexpr block_iterator create_storable_block(const size_type size)
		{
			pool_.emplace_back(std::ranges::max(capacity_, size + IsNullTerminate));
			return std::ranges::prev(pool_.end());
		}

		[[nodiscard]] constexpr block_iterator create_storable_block(const view_type str) { return this->create_storable_block(str.size()); }

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
		constexpr explicit string_pool(Pools&&... pools) { this->append(std::forward<Pools>(pools)...); }

		template<std::same_as<string_pool>... Pools>
		constexpr void append(Pools&&... pools)
		{
			pool_.reserve(pool_.size() + (pools.pool_.size() + ...));

			block_iterator iterator;
			(((iterator = pool_.insert(pool_.end(), std::make_move_iterator(pools.pool_.begin()), std::make_move_iterator(pools.pool_.end()))),
			  pools.pool_.clear(),
			  std::ranges::inplace_merge(pool_.begin(), iterator, pool_.end(), [](const auto& a, const auto& b) { return not a.more_available_space_than(b); })),
				...);
		}

		/**
		 * @brief Add a string to the pool, and then you can freely use the added string.
		 */
		constexpr view_type append(const view_type str) { return this->append_str_into_block(str, this->find_or_create_block(str)); }

		/**
		 * @brief Borrow a block of memory to the pool, users can directly write strings in this memory area without worrying about its invalidation.
		 */
		[[nodiscard]] constexpr value_type* borrow_raw(const size_type size) { return this->borrow_raw_memory(size, this->find_or_create_block(size)); }

		/**
		 * @brief User needs to temporarily use some memory area to store the string and return it later
		 */
		[[nodiscard]] constexpr block_borrower borrow_block() noexcept { return block_borrower{*this}; }

		[[nodiscard]] constexpr size_type size() const noexcept { return pool_.size(); }

		[[nodiscard]] constexpr size_type capacity() const noexcept { return capacity_; }

		/**
		 * @note Only affect the block created after modification
		 */
		constexpr void resize(size_type capacity) noexcept { capacity_ = capacity; }
	};
}

#endif // GAL_UTILS_STRING_POOL_HPP
