#pragma once

#include <utils/assert.hpp>

namespace gal::utils
{
	/**
	 * @brief an iterator with counters that supports jumping
	 * @tparam Stride number of steps per jump
	 * @tparam Iterator iterator type (at least support input_or_output)
	*/
	template<std::size_t Stride, std::input_or_output_iterator Iterator>
	class stride_iterator
	{
	public:
		constexpr static auto stride = Stride;

		using iterator_type = Iterator;
		using value_type = std::iter_value_t<iterator_type>;
		using difference_type = std::iter_difference_t<iterator_type>;
		using reference = std::iter_reference_t<iterator_type>;
		using rreference = std::iter_rvalue_reference_t<iterator_type>;

		static_assert(std::is_signed_v<difference_type>);

	private:
		/**
		 * @brief move base iterator and corresponding change count
		 * @param step number of moves
		*/
		constexpr void move_base(difference_type step) noexcept(noexcept(std::advance(
				std::declval<const iterator_type>(),
				std::declval<difference_type>())))
		{
			std::advance(base_, step * stride);
			count_ -= step;
		}

	public:
		/**
		 * @brief all behaviors produced by using the default construction are undefined, and the only reason for the existence of this constructor is as follows
		 * @see https://wg21.link/P2325R3
		*/
		constexpr stride_iterator() = default;

		/**
		 * @brief construct a stride_iterator and specify count
		 * @param iterator base iterator
		 * @param count count
		*/
		constexpr explicit stride_iterator(iterator_type iterator, difference_type count) noexcept(std::is_nothrow_move_constructible_v<iterator_type>)
			: base_(std::move(iterator)),
			  count_(count) { gal_assert(count >= 0, "stride_iterator requires non-negative length n"); }

		/**
		 * @brief copy construct a stride_iterator
		 * @param other the object to be copied (must be the same as the stride of the other party)
		*/
		template<std::input_or_output_iterator I>
			requires std::convertible_to<const I&, iterator_type>
		constexpr explicit stride_iterator(const stride_iterator<stride, I>& other) noexcept(std::is_nothrow_constructible_v<iterator_type, const I&>)
			: base_(other.base()),
			  count_(other.count()) {}

		/**
		 * @brief copy assign a stride_iterator
		 * @param other other the object to be copied (must be the same as the stride of the other party)
		 * @return copied object
		*/
		template<std::input_or_output_iterator I>
			requires std::assignable_from<iterator_type&, const I&>
		constexpr stride_iterator& operator=(const stride_iterator<stride, I>& other) noexcept(std::is_nothrow_assignable_v<iterator_type, const I&>)
		{
			base_ = other.base();
			count_ = other.count();
			return *this;
		}

		/**
		 * @brief get the underlying iterator
		 */
		constexpr const iterator_type& base() const & noexcept { return base_; }

		/**
		 * @brief get the underlying iterator
		 */
		[[nodiscard]] constexpr iterator_type base() && noexcept(std::is_nothrow_move_constructible_v<iterator_type>) { return std::move(base_); }

		/**
		 * @brief get the count
		 */
		[[nodiscard]] constexpr difference_type count() const noexcept { return count_; }

		[[nodiscard]] constexpr decltype(auto) operator*() noexcept(noexcept(*std::declval<iterator_type>()))
		{
			gal_assert(count_ >= 0, "stride_iterator dereference beyond end of range");
			return *base_;
		}

		[[nodiscard]] constexpr decltype(auto) operator*() const
		noexcept(noexcept(*std::declval<iterator_type>()))
		{
			gal_assert(count_ >= 0, "stride_iterator dereference beyond end of range");
			return *base_;
		}

		[[nodiscard]] constexpr decltype(auto) operator[](difference_type n) const
		noexcept(noexcept(std::declval<iterator_type>()[0])) requires std::random_access_iterator<iterator_type>
		{
			gal_assert(n < count_, "stride_iterator index out of range");
			return base_[n * stride];
		}

		// stl compatible, can also be used for for-loop
		[[nodiscard]] constexpr stride_iterator begin() const noexcept { return {*this}; }

		// ReSharper disable once CppMemberFunctionMayBeStatic
		// stl compatible, can also be used for for-loop
		[[nodiscard]] constexpr std::default_sentinel_t end() const noexcept { return std::default_sentinel; }

		constexpr stride_iterator& operator++() noexcept(noexcept(move_base(std::declval<difference_type>())))
		{
			gal_assert(count_ > 0, "stride_iterator increment beyond end of range");
			move_base(1);
			return *this;
		}

		[[nodiscard]] constexpr stride_iterator operator++(int) noexcept(noexcept(move_base(std::declval<difference_type>()))) requires std::forward_iterator<iterator_type>
		{
			auto copy{*this};
			move_base(1);
			return copy;
		}

		constexpr stride_iterator& operator--() noexcept(noexcept(move_base(std::declval<difference_type>()))) requires std::bidirectional_iterator<iterator_type>
		{
			move_base(-1);
			return *this;
		}

		[[nodiscard]] constexpr stride_iterator operator--(int) noexcept(noexcept(move_base(std::declval<difference_type>()))) requires std::bidirectional_iterator<iterator_type>
		{
			auto copy{*this};
			move_base(-1);
			return copy;
		}

		[[nodiscard]] constexpr stride_iterator operator+(difference_type n) const
		noexcept(noexcept(std::declval<iterator_type>() + std::declval<difference_type>())) requires std::random_access_iterator<iterator_type> { return stride_iterator{base_ + n, static_cast<difference_type>(count_ - n)}; }

		[[nodiscard]] constexpr friend stride_iterator operator+(difference_type n, const stride_iterator& it) noexcept(noexcept(std::declval<iterator_type>() + std::declval<difference_type>())) requires std::random_access_iterator<iterator_type> { return stride_iterator{it.base_ + n, static_cast<difference_type>(it.count_ - n)}; }

		constexpr stride_iterator& operator+=(difference_type n) noexcept(noexcept(move_base(std::declval<difference_type>()))) requires std::random_access_iterator<iterator_type>
		{
			gal_assert(n <= count_, "stride_iterator seek beyond end of range");
			move_base(n);
			return *this;
		}

		[[nodiscard]] constexpr stride_iterator operator-(difference_type n) const
		noexcept(noexcept(std::declval<iterator_type>() - std::declval<difference_type>())) requires std::random_access_iterator<iterator_type> { return stride_iterator{base_ - n, static_cast<difference_type>(count_ + n)}; }

		template<std::common_with<iterator_type> I>
		[[nodiscard]] constexpr friend difference_type operator-(
				const stride_iterator& lhs,
				const stride_iterator<stride, I>& rhs) noexcept { return rhs.count() - lhs.count_; }

		[[nodiscard]] constexpr friend difference_type operator-(
				const stride_iterator& lhs,
				std::default_sentinel_t) noexcept { return -lhs.count_; }

		[[nodiscard]] constexpr friend difference_type operator-(
				std::default_sentinel_t,
				const stride_iterator& rhs) noexcept { return rhs.count_; }

		constexpr stride_iterator& operator-=(difference_type n) noexcept(noexcept(std::declval<iterator_type>() -= 1)) requires std::random_access_iterator<iterator_type>
		{
			gal_assert(-n <= count_, "stride_iterator decrement beyond end of range");
			move_base(-n);
			return *this;
		}

		template<std::common_with<iterator_type> I>
		[[nodiscard]] constexpr friend bool operator==(
				const stride_iterator& lhs,
				const stride_iterator<stride, I>& rhs) noexcept { return lhs.count_ == rhs.count(); }

		[[nodiscard]] constexpr friend bool operator==(
				const stride_iterator& lhs,
				std::default_sentinel_t) noexcept { return lhs.count_ == 0; }

		template<std::common_with<iterator_type> I>
		[[nodiscard]] constexpr friend std::strong_ordering operator<=>(
				const stride_iterator& lhs,
				const stride_iterator<stride, I>& rhs) noexcept { return rhs.count() <=> lhs.count_; }

	private:
		iterator_type base_;
		difference_type count_{};

	public:
		[[nodiscard]] constexpr friend rreference iter_move(const stride_iterator& other) noexcept(noexcept(std::ranges::iter_move(other.base_))) requires std::input_iterator<iterator_type> { return std::ranges::iter_move(other.base_); }

		template<std::indirectly_swappable<iterator_type> I>
		constexpr friend void iter_swap(const stride_iterator& lhs, const stride_iterator<stride, I>& rhs) noexcept(noexcept(std::ranges::iter_swap(lhs.base_, rhs.base()))) { return std::ranges::iter_swap(lhs.base_, rhs.base()); }
	};

	template<std::size_t Stride, std::random_access_iterator Iterator>
	constexpr stride_iterator<Stride, Iterator> make_stride_iterator(
			Iterator iterator,
			typename stride_iterator<Stride, Iterator>::difference_type count) noexcept(std::is_nothrow_constructible_v<stride_iterator<Stride, Iterator>,
			                                                                                                            Iterator,
			                                                                                                            typename stride_iterator<Stride, Iterator>::difference_type>) { return stride_iterator<Stride, Iterator>{iterator, count}; }
}// namespace gal::toolbox::iterator

namespace std
{
	template<std::size_t Stride, typename Iterator>
	struct incrementable_traits<gal::utils::stride_iterator<Stride, Iterator>>
	{
		using difference_type = typename gal::utils::stride_iterator<Stride, Iterator>::difference_type;
	};

	template<std::size_t Stride, std::input_iterator Iterator>
	struct iterator_traits<gal::utils::stride_iterator<Stride, Iterator>> : iterator_traits<Iterator>
	{
		using pointer = void;
	};

	template<std::size_t Stride, contiguous_iterator Iterator>
	struct pointer_traits<gal::utils::stride_iterator<Stride, Iterator>>
	{
		using pointer = gal::utils::stride_iterator<Stride, Iterator>;
		using element_type = remove_reference_t<typename gal::utils::stride_iterator<Stride, Iterator>::reference>;
		using difference_type = typename gal::utils::stride_iterator<Stride, Iterator>::difference_type;

		[[nodiscard]] constexpr static element_type* to_address(const pointer iterator) noexcept { return std::to_address(iterator.base()); }
	};
}// namespace std
