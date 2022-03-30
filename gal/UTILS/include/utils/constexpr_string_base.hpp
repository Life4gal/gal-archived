#pragma once

#ifndef GAL_UTILS_CONSTEXPR_STRING_BASE_HPP
#define GAL_UTILS_CONSTEXPR_STRING_BASE_HPP

#include <type_traits>

namespace gal::utils
{
	template<typename Derived, typename ValueType, typename SizeType = std::size_t>
	struct constexpr_string_base
	{
		using derived_type = std::remove_cvref_t<Derived>;

		using value_type = ValueType;
		using size_type = SizeType;

		[[nodiscard]] constexpr static bool match(const value_type* string) noexcept
			requires requires
			{
				derived_type::size_no_0;
			}
		{
			return std::char_traits<value_type>::length(string) == derived_type::size_no_0 &&
			       std::char_traits<value_type>::compare(derived_type::value, string, derived_type::size_no_0) == 0;
		}

		[[nodiscard]] constexpr bool match(const value_type* string) const noexcept
			requires std::is_member_function_pointer_v<decltype(&derived_type::size_no_0)>
		{
			return std::char_traits<value_type>::length(string) == static_cast<const derived_type&>(*this).size_no_0() &&
			       std::char_traits<value_type>::compare(static_cast<const derived_type&>(*this).value, string, static_cast<const derived_type&>(*this).size_no_0()) == 0;
		}

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<template<typename...> typename Container, typename CharGetter, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr static bool match(
				const Container<value_type, AnyOther...>& container,
				CharGetter getter,
				CharComparator comparator = CharComparator{})
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container<value_type, AnyOther...>&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
				         {
					         comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				         } -> std::convertible_to<bool>;
			         } &&
			         requires
			         {
				         derived_type::size_no_0;
				         derived_type::value;
			         } { return container.size() == derived_type::size_no_0 && [&]<std::size_t... Index>(std::index_sequence<Index...>) { return ((comparator(getter(container, Index), derived_type::value[Index])) && ...); }(std::make_index_sequence<derived_type::size_no_0>{}); }

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<template<typename...> typename Container, typename CharGetter, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr bool match(
				const Container<value_type, AnyOther...>& container,
				CharGetter getter,
				CharComparator comparator = CharComparator{}) const
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container<value_type, AnyOther...>&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				} -> std::convertible_to<bool>;
			} && std::is_member_function_pointer_v<decltype(&derived_type::size_no_0)>
		{
			if (container.size() != static_cast<const derived_type&>(*this).size_no_0()) { return false; }

			for (decltype(container.size()) i = 0; i < container.size(); ++i) { if (not comparator(getter(container, i), static_cast<const derived_type&>(*this).value[i])) { return false; } }

			return true;
		}

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<typename Container, typename CharGetter, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr static bool match(
				const Container& container,
				CharGetter getter,
				CharComparator comparator = CharComparator{})
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
				         {
					         comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				         } -> std::convertible_to<bool>;
			         } &&
			         requires
			         {
				         derived_type::size_no_0;
				         derived_type::value;
			         } { return container.size() == derived_type::size_no_0 && [&]<std::size_t... Index>(std::index_sequence<Index...>) { return ((comparator(getter(container, Index), derived_type::value[Index])) && ...); }(std::make_index_sequence<derived_type::size_no_0>{}); }

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<typename Container, typename CharGetter, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr bool match(
				const Container& container,
				CharGetter getter,
				CharComparator comparator = CharComparator{}) const
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				} -> std::convertible_to<bool>;
			} && std::is_member_function_pointer_v<decltype(&derived_type::size_no_0)>
		{
			if (container.size() != static_cast<const derived_type&>(*this).size_no_0()) { return false; }

			for (decltype(container.size()) i = 0; i < container.size(); ++i) { if (not comparator(getter(container, i), static_cast<const derived_type&>(*this).value[i])) { return false; } }

			return true;
		}

		template<template<typename...> typename Container, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr static bool match(
				const Container<value_type, AnyOther...>& container,
				CharComparator comparator = {})
		noexcept(noexcept(container[std::declval<size_type>()]) &&
		         noexcept(
			         constexpr_string_base::match(
					         container,
					         [](const auto& c, size_type index) { return c[index]; },
					         comparator)))
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
				         {
					         container[std::declval<size_type>()]
				         } -> std::same_as<value_type>;
			         } &&
			         requires
			         {
				         derived_type::size_no_0;
				         derived_type::value;
			         }
		{
			return constexpr_string_base::match(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}

		template<template<typename...> typename Container, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr bool match(
				const Container<value_type, AnyOther...>& container,
				CharComparator comparator = {}) const
		noexcept(noexcept(container[std::declval<size_type>()]) &&
		         noexcept(std::declval<const constexpr_string_base&>.match(
				         container,
				         [](const auto& c, size_type index) { return c[index]; },
				         comparator)))
			requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					container[std::declval<size_type>()]
				} -> std::same_as<value_type>;
			} && std::is_member_function_pointer_v<decltype(&derived_type::size_no_0)>
		{
			return this->match(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}

		template<typename Container, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr static bool match(
				const Container& container,
				CharComparator comparator = {})
		noexcept(noexcept(container[std::declval<size_type>()]) &&
		         noexcept(
			         constexpr_string_base::match(
					         container,
					         [](const auto& c, size_type index) { return c[index]; },
					         comparator)))
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
			         } &&
			         requires
			         {
				         derived_type::size_no_0;
				         derived_type::value;
			         }
		{
			return constexpr_string_base::match(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}

		template<typename Container, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr bool match(
				const Container& container,
				CharComparator comparator = {}) const
		noexcept(noexcept(container[std::declval<size_type>()]) &&
		         noexcept(std::declval<const constexpr_string_base&>.match(
				         container,
				         [](const auto& c, size_type index) { return c[index]; },
				         comparator)))
			requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
			} && std::is_member_function_pointer_v<decltype(&derived_type::size_no_0)>
		{
			return this->match(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}
	};

	template<typename Derived, typename LeftType, typename RightType, typename ValueType, typename SizeType = std::size_t>
		requires std::is_base_of_v<constexpr_string_base<LeftType, ValueType, SizeType>, LeftType> && std::is_base_of_v<constexpr_string_base<RightType, ValueType, SizeType>, RightType>
	struct bilateral_constexpr_string_base
	{
		using derived_type = Derived;

		using left_type = LeftType;
		using right_type = RightType;

		using value_type = typename left_type::template value_type;
		using size_type = typename left_type::template size_type;

		[[nodiscard]] constexpr static bool match_left(const value_type* string) noexcept
			requires requires
			{
				left_type::size_no_0;
			} { return left_type::match(string); }

		[[nodiscard]] constexpr bool match_left(const value_type* string) const noexcept
			requires std::is_member_function_pointer_v<decltype(&left_type::size_no_0)> { return static_cast<const derived_type&>(*this).left_value.match(string); }

		[[nodiscard]] constexpr static bool match_right(const value_type* string) noexcept
			requires requires
			{
				right_type::size_no_0;
			} { return right_type::match(string); }

		[[nodiscard]] constexpr bool match_right(const value_type* string) const noexcept
			requires std::is_member_function_pointer_v<decltype(&right_type::size_no_0)> { return static_cast<const derived_type&>(*this).right_value.match(string); }

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<template<typename...> typename Container, typename CharGetter, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr static bool match_left(
				const Container<value_type, AnyOther...>& container,
				CharGetter&& getter,
				CharComparator&& comparator = CharComparator{})
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container<value_type, AnyOther...>&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
				         {
					         comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				         } -> std::convertible_to<bool>;
			         } &&
			         requires
			         {
				         left_type::size_no_0;
				         left_type::value;
			         } { return left_type::match(container, std::forward<CharGetter>(getter), std::forward<CharComparator>(comparator)); }

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<template<typename...> typename Container, typename CharGetter, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr bool match_left(
				const Container<value_type, AnyOther...>& container,
				CharGetter&& getter,
				CharComparator&& comparator = CharComparator{}) const
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container<value_type, AnyOther...>&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				} -> std::convertible_to<bool>;
			} && std::is_member_function_pointer_v<decltype(&left_type::size_no_0)> { return static_cast<const derived_type&>(*this).left_value.match(container, std::forward<CharGetter>(getter), std::forward<CharComparator>(comparator)); }

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<template<typename...> typename Container, typename CharGetter, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr static bool match_right(
				const Container<value_type, AnyOther...>& container,
				CharGetter&& getter,
				CharComparator&& comparator = CharComparator{})
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container<value_type, AnyOther...>&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
				         {
					         comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				         } -> std::convertible_to<bool>;
			         } &&
			         requires
			         {
				         right_type::size_no_0;
				         right_type::value;
			         } { return right_type::match(container, std::forward<CharGetter>(getter), std::forward<CharComparator>(comparator)); }

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<template<typename...> typename Container, typename CharGetter, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr bool match_right(
				const Container<value_type, AnyOther...>& container,
				CharGetter&& getter,
				CharComparator&& comparator = CharComparator{}) const
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container<value_type, AnyOther...>&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>) requires requires
		{
			{
				container.size()
			} -> std::convertible_to<size_type>;
			{
				comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
			} -> std::convertible_to<bool>;
		} && std::is_member_function_pointer_v<decltype(&right_type::size_no_0)> { return static_cast<const derived_type&>(*this).right_value.match(container, std::forward<CharGetter>(getter), std::forward<CharComparator>(comparator)); }

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<typename Container, typename CharGetter, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr static bool match_left(
				const Container& container,
				CharGetter&& getter,
				CharComparator&& comparator = CharComparator{})
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
				         {
					         comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				         } -> std::convertible_to<bool>;
			         } &&
			         requires
			         {
				         left_type::size_no_0;
				         left_type::value;
			         } { return left_type::match(container, std::forward<CharGetter>(getter), std::forward<CharComparator>(comparator)); }

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<typename Container, typename CharGetter, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr bool match_left(
				const Container& container,
				CharGetter&& getter,
				CharComparator&& comparator = CharComparator{}) const
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				} -> std::convertible_to<bool>;
			} && std::is_member_function_pointer_v<decltype(&left_type::size_no_0)> { return static_cast<const derived_type&>(*this).left_value.match(container, std::forward<CharGetter>(getter), std::forward<CharComparator>(comparator)); }

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<typename Container, typename CharGetter, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr static bool match_right(
				const Container& container,
				CharGetter&& getter,
				CharComparator&& comparator = CharComparator{})
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
				         {
					         comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				         } -> std::convertible_to<bool>;
			         } &&
			         requires
			         {
				         right_type::size_no_0;
				         right_type::value;
			         } { return right_type::match(container, std::forward<CharGetter>(getter), std::forward<CharComparator>(comparator)); }

		/**
		 * @note CharGetter' s parameters are the indices of value
		 * @note CharComparator' s first parameter is container's element 
		 */
		template<typename Container, typename CharGetter, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr bool match_right(
				const Container& container,
				CharGetter&& getter,
				CharComparator&& comparator = CharComparator{}) const
		noexcept(std::is_nothrow_invocable_v<CharGetter, const Container&, size_type> &&
		         std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
			requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				} -> std::convertible_to<bool>;
			} && std::is_member_function_pointer_v<decltype(&right_type::size_no_0)> { return static_cast<const derived_type&>(*this).right_value.match(container, std::forward<CharGetter>(getter), std::forward<CharComparator>(comparator)); }

		template<template<typename...> typename Container, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr static bool match_left(
				const Container<value_type, AnyOther...>& container,
				CharComparator comparator = {})
		noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(bilateral_constexpr_string_base::match_left(
				         container,
				         [](const auto& c, size_type index) { return c[index]; },
				         comparator)))
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
				         {
					         container[std::declval<size_type>()]
				         } -> std::same_as<value_type>;
			         } &&
			         requires
			         {
				         left_type::size_no_0;
				         left_type::value;
			         }
		{
			return bilateral_constexpr_string_base::match_left(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}

		template<template<typename...> typename Container, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr bool match_left(
				const Container<value_type, AnyOther...>& container,
				CharComparator comparator = {}) const
		noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(std::declval<const bilateral_constexpr_string_base&>.match_left(
				         container,
				         [](const auto& c, size_type index) { return c[index]; },
				         comparator)))
			requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					container[std::declval<size_type>()]
				} -> std::same_as<value_type>;
			} && std::is_member_function_pointer_v<decltype(&left_type::size_no_0)>
		{
			return this->match_left(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}

		template<template<typename...> typename Container, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr static bool match_right(
				const Container<value_type, AnyOther...>& container,
				CharComparator comparator = {})
		noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(bilateral_constexpr_string_base::match_right(
				         container,
				         [](const auto& c, size_type index) { return c[index]; },
				         comparator)))
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
				         {
					         container[std::declval<size_type>()]
				         } -> std::same_as<value_type>;
			         } &&
			         requires
			         {
				         right_type::size_no_0;
				         right_type::value;
			         }
		{
			return bilateral_constexpr_string_base::match_right(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}

		template<template<typename...> typename Container, typename CharComparator = std::equal_to<>, typename... AnyOther>
		[[nodiscard]] constexpr bool match_right(
				const Container<value_type, AnyOther...>& container,
				CharComparator comparator = {}) const
		noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(std::declval<const bilateral_constexpr_string_base&>.match_right(
				         container,
				         [](const auto& c, size_type index) { return c[index]; },
				         comparator)))
			requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					container[std::declval<size_type>()]
				} -> std::same_as<value_type>;
			} && std::is_member_function_pointer_v<decltype(&right_type::size_no_0)>
		{
			return this->match_right(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}

		template<typename Container, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr static bool match_left(
				const Container& container,
				CharComparator comparator = {})
		noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(bilateral_constexpr_string_base::match_left(
				         container,
				         [](const auto& c, size_type index) { return c[index]; },
				         comparator)))
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
			         } &&
			         requires
			         {
				         left_type::size_no_0;
				         left_type::value;
			         }
		{
			return bilateral_constexpr_string_base::match_left(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}

		template<typename Container, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr bool match_left(
				const Container& container,
				CharComparator comparator = {}) const
		noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(std::declval<const bilateral_constexpr_string_base&>.match_left(
				         container,
				         [](const auto& c, size_type index) { return c[index]; },
				         comparator)))
			requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
			} && std::is_member_function_pointer_v<decltype(&left_type::size_no_0)>
		{
			return this->match_left(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}

		template<typename Container, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr static bool match_right(
				const Container& container,
				CharComparator comparator = {})
		noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(bilateral_constexpr_string_base::match_right(
				         container,
				         [](const auto& c, size_type index) { return c[index]; },
				         comparator)))
			requires requires
			         {
				         {
					         container.size()
				         } -> std::convertible_to<size_type>;
			         } &&
			         requires
			         {
				         right_type::size_no_0;
				         right_type::value;
			         }
		{
			return bilateral_constexpr_string_base::match_right(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}

		template<typename Container, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
		[[nodiscard]] constexpr bool match_right(
				const Container& container,
				CharComparator comparator = {}) const
		noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(std::declval<const bilateral_constexpr_string_base&>.match_right(
				         container,
				         [](const auto& c, size_type index) { return c[index]; },
				         comparator))) requires requires
		{
			{
				container.size()
			} -> std::convertible_to<size_type>;
		} && std::is_member_function_pointer_v<decltype(&right_type::size_no_0)>
		{
			return this->match_right(
					container,
					[](const auto& c, size_type index) { return c[index]; },
					comparator);
		}
	};
}

#endif // GAL_UTILS_CONSTEXPR_STRING_BASE_HPP
