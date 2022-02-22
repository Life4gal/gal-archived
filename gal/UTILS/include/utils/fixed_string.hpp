#pragma once

#ifndef GAL_UTILS_FIXED_STRING_HPP
#define GAL_UTILS_FIXED_STRING_HPP

namespace gal::utils
{
	namespace detail
	{
		template<typename T, T... Chars>
		struct basic_fixed_string
		{
			using value_type = T;
			using size_type = std::size_t;

			using const_iterator = const value_type*;

			constexpr static size_type size = sizeof...(Chars);// include '\0'
			constexpr static size_type size_no_0 = size - 1;
			constexpr static value_type value[size]{Chars...};

			[[nodiscard]] constexpr static bool match(const value_type* string) noexcept
			{
				return std::char_traits<value_type>::length(string) == size_no_0 &&
				       std::char_traits<value_type>::compare(value, string, size_no_0) == 0;
			}

			/**
			 * @note CharGetter' s parameters are the indices of value
			 * @note CharCompare' s first parameter is container's element 
			 */
			template<template<typename...> typename Container, typename CharGetter, typename CharCompare = std::equal_to<>, typename... AnyOther>
			[[nodiscard]] constexpr static bool match(
					const Container<value_type, AnyOther...>& container,
					CharGetter getter,
					CharCompare compare = CharCompare{}
					)
			noexcept(
				std::is_nothrow_invocable_v<CharGetter, const Container<value_type, AnyOther...>&, size_type> &&
				std::is_nothrow_invocable_v<CharCompare, decltype(getter(container, std::declval<size_type>())), value_type>)
				requires requires
				{
					{
						container.size()
					} -> std::convertible_to<size_type>;
					{
						compare(getter(container, std::declval<size_type>()), std::declval<value_type>())
					} -> std::convertible_to<bool>;
				} { return container.size() == size_no_0 && [&]<std::size_t... Index>(std::index_sequence<Index...>) { return ((compare(getter(container, Index), value[Index])) && ...); }(std::make_index_sequence<size_no_0>{}); }

			/**
			 * @note CharGetter' s parameters are the indices of value
			 * @note CharCompare' s first parameter is container's element 
			 */
			template<typename Container, typename CharGetter, typename CharCompare = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
			[[nodiscard]] constexpr static bool match(
					const Container& container,
					CharGetter getter,
					CharCompare compare = CharCompare{}
					)
			noexcept(
				std::is_nothrow_invocable_v<CharGetter, const Container&, size_type> &&
				std::is_nothrow_invocable_v<CharCompare, decltype(getter(container, std::declval<size_type>())), value_type>)
				requires requires
				{
					{
						container.size()
					} -> std::convertible_to<size_type>;
					{
						compare(getter(container, std::declval<size_type>()), std::declval<value_type>())
					} -> std::convertible_to<bool>;
				} { return container.size() == size_no_0 && [&]<std::size_t... Index>(std::index_sequence<Index...>) { return ((compare(getter(container, Index), value[Index])) && ...); }(std::make_index_sequence<size_no_0>{}); }

			template<template<typename...> typename Container, typename CharCompare = std::equal_to<>, typename... AnyOther>
			[[nodiscard]] constexpr static bool match(
					const Container<value_type, AnyOther...>& container,
					CharCompare compare = {}
					)
			noexcept(
				noexcept(container[std::declval<size_type>()]) &&
				noexcept(match(container, [](const auto& c, size_type index) { return c[index]; }, compare))
			)
				requires requires
				{
					{
						container.size()
					} -> std::convertible_to<size_type>;
					{
						container[std::declval<size_type>()]
					} -> std::same_as<value_type>;
				}
			{
				return match(
						container,
						[](const auto& c, size_type index) { return c[index]; },
						compare
						);
			}

			template<typename Container, typename CharCompare = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
			[[nodiscard]] constexpr static bool match(
					const Container& container,
					CharCompare compare = {}
					)
			noexcept(
				noexcept(container[std::declval<size_type>()]) &&
				noexcept(match(
						container,
						[](const auto& c, size_type index) { return c[index]; },
						compare
						)
				)
			)
				requires requires
				{
					{
						container.size()
					} -> std::convertible_to<size_type>;
				}
			{
				return match(
						container,
						[](const auto& c, size_type index) { return c[index]; },
						compare
						);
			}

			[[nodiscard]] constexpr static const_iterator begin() noexcept { return std::begin(value); }

			[[nodiscard]] constexpr static const_iterator end() noexcept { return std::end(value); }
		};
	}

	template<char... Chars>
	using fixed_string = detail::basic_fixed_string<char, Chars...>;
	template<wchar_t... Chars>
	using fixed_wstring = detail::basic_fixed_string<wchar_t, Chars...>;
	template<char8_t... Chars>
	// ReSharper disable once CppInconsistentNaming
	using fixed_u8string = detail::basic_fixed_string<char8_t, Chars...>;
	template<char16_t... Chars>
	// ReSharper disable once CppInconsistentNaming
	using fixed_u16string = detail::basic_fixed_string<char16_t, Chars...>;
	template<char32_t... Chars>
	// ReSharper disable once CppInconsistentNaming
	using fixed_u32string = detail::basic_fixed_string<char32_t, Chars...>;

	#define GAL_UTILS_DO_NOT_USE_FIXED_STRING_TYPE_GENERATOR(string_type, string) \
	decltype(\
			[]<std::size_t... Index>(std::index_sequence<Index...>) constexpr noexcept {\
				return ::gal::utils::string_type<[](std::size_t index) constexpr noexcept\
				{\
					return (string)[index];\
				}\
				(Index)... > {};\
			}(std::make_index_sequence<sizeof(string) / sizeof((string)[0])>()))


	#define GAL_UTILS_FIXED_STRING_TYPE(string) GAL_UTILS_DO_NOT_USE_FIXED_STRING_TYPE_GENERATOR(fixed_string, string)
	#define GAL_UTILS_FIXED_WSTRING_TYPE(string) GAL_UTILS_DO_NOT_USE_FIXED_STRING_TYPE_GENERATOR(fixed_wstring, string)
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_FIXED_U8STRING_TYPE(string) GAL_UTILS_DO_NOT_USE_FIXED_STRING_TYPE_GENERATOR(fixed_u8string, string)
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_FIXED_U16STRING_TYPE(string) GAL_UTILS_DO_NOT_USE_FIXED_STRING_TYPE_GENERATOR(fixed_u16string, string)
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_FIXED_U32STRING_TYPE(string) GAL_UTILS_DO_NOT_USE_FIXED_STRING_TYPE_GENERATOR(fixed_u32string, string)
}

#endif // GAL_UTILS_FIXED_STRING_HPP
