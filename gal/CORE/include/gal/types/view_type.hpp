#pragma once

#ifndef GAL_LANG_TYPES_VIEW_TYPE_HPP
#define GAL_LANG_TYPES_VIEW_TYPE_HPP

#include <gal/foundation/type_info.hpp>

namespace gal::lang
{
	namespace foundation
	{
		class boxed_value;
	}

	namespace types
	{
		// see also foundation/name.hpp => container_view_xxx_interface_name
		template<typename ContainerType>
		class view_type
		{
		public:
			using container_type = ContainerType;

			constexpr static bool is_const_container = std::is_const_v<container_type>;

			using value_type = typename container_type::value_type;
			using iterator_type = std::conditional_t<is_const_container, typename container_type::const_iterator, typename container_type::iterator>;

			static const foundation::gal_type_info& class_type() noexcept
			{
				GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,)
				static foundation::gal_type_info type = foundation::make_type_info<view_type<container_type>>();
				return type;
			}

		private:
			iterator_type begin_;
			iterator_type end_;

		public:
			constexpr explicit view_type(container_type& container) noexcept
				requires(not is_const_container)
				: begin_{std::ranges::begin(container)},
				  end_{std::ranges::end(container)} {}

			constexpr explicit view_type(const container_type& container) noexcept
				requires is_const_container
				: begin_{std::ranges::begin(container)},
				  end_{std::ranges::end(container)} {}

			[[nodiscard]] constexpr bool empty() const noexcept { return begin_ == end_; }

			[[nodiscard]] constexpr foundation::boxed_value get() noexcept
			// requires(not is_const_container)
			{
				if constexpr (std::is_same_v<value_type, foundation::boxed_value>) { return *begin_; }
				else { return foundation::boxed_value{std::ref(*begin_)}; }
			}

			[[nodiscard]] constexpr foundation::boxed_value get() const noexcept
			{
				if constexpr (std::is_same_v<value_type, foundation::boxed_value>) { return *begin_; }
				else { return foundation::boxed_value{std::cref(*begin_)}; }
			}

			constexpr void advance() noexcept { std::ranges::advance(begin_, 1); }
		};
	}
}

#endif // GAL_LANG_TYPES_VIEW_TYPE_HPP
