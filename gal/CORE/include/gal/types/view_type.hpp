#pragma once

#ifndef GAL_LANG_TYPES_VIEW_TYPE_HPP
#define GAL_LANG_TYPES_VIEW_TYPE_HPP

namespace gal::lang::types
{
	template<typename ContainerType>
	class view_type
	{
	public:
		using container_type = ContainerType;

		constexpr static bool is_const_container = std::is_const_v<container_type>;

		using iterator_type = std::conditional_t<is_const_container, typename container_type::const_iterator, typename container_type::iterator>;

	private:
		iterator_type begin_;
		iterator_type end_;

	public:
		constexpr explicit view_type(container_type& container)
			: begin_{std::ranges::begin(container)},
			  end_{std::ranges::end(container)} {}

		[[nodiscard]] constexpr bool empty() const noexcept { return begin_ == end_; }

		constexpr void pop_front()
		{
			if (empty()) { throw std::range_error{"empty view"}; }
			++begin_;
		}

		constexpr void pop_back()
		{
			if (empty()) { throw std::range_error{"empty view"}; }
			--end_;
		}

		[[nodiscard]] constexpr decltype(auto) front() const
		{
			if (empty()) { throw std::range_error{"empty view"}; }
			return *begin_;
		}

		[[nodiscard]] constexpr decltype(auto) back() const
		{
			if (empty()) { throw std::range_error{"empty view"}; }
			return *std::ranges::prev(end_);
		}
	};
}

#endif // GAL_LANG_TYPES_VIEW_TYPE_HPP
