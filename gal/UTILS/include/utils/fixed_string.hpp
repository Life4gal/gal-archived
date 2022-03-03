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
			 * @note CharComparator' s first parameter is container's element 
			 */
			template<template<typename...> typename Container, typename CharGetter, typename CharComparator = std::equal_to<>, typename... AnyOther>
			[[nodiscard]] constexpr static bool match(
					const Container<value_type, AnyOther...>& container,
					CharGetter getter,
					CharComparator comparator = CharComparator{}
					)
			noexcept(
				std::is_nothrow_invocable_v<CharGetter, const Container<value_type, AnyOther...>&, size_type> &&
				std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
				requires requires
				{
					{
						container.size()
					} -> std::convertible_to<size_type>;
					{
						comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
					} -> std::convertible_to<bool>;
				} { return container.size() == size_no_0 && [&]<std::size_t... Index>(std::index_sequence<Index...>) { return ((comparator(getter(container, Index), value[Index])) && ...); }(std::make_index_sequence<size_no_0>{}); }

			/**
			 * @note CharGetter' s parameters are the indices of value
			 * @note CharComparator' s first parameter is container's element 
			 */
			template<typename Container, typename CharGetter, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
			[[nodiscard]] constexpr static bool match(
					const Container& container,
					CharGetter getter,
					CharComparator comparator = CharComparator{}
					)
			noexcept(
				std::is_nothrow_invocable_v<CharGetter, const Container&, size_type> &&
				std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>)
				requires requires
				{
					{
						container.size()
					} -> std::convertible_to<size_type>;
					{
						comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
					} -> std::convertible_to<bool>;
				} { return container.size() == size_no_0 && [&]<std::size_t... Index>(std::index_sequence<Index...>) { return ((comparator(getter(container, Index), value[Index])) && ...); }(std::make_index_sequence<size_no_0>{}); }

			template<template<typename...> typename Container, typename CharComparator = std::equal_to<>, typename... AnyOther>
			[[nodiscard]] constexpr static bool match(
					const Container<value_type, AnyOther...>& container,
					CharComparator comparator = {}
					)
			noexcept(
				noexcept(container[std::declval<size_type>()]) &&
				noexcept(match(container, [](const auto& c, size_type index) { return c[index]; }, comparator))
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
						comparator
						);
			}

			template<typename Container, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
			[[nodiscard]] constexpr static bool match(
					const Container& container,
					CharComparator comparator = {}
					)
			noexcept(
				noexcept(container[std::declval<size_type>()]) &&
				noexcept(match(
						container,
						[](const auto& c, size_type index) { return c[index]; },
						comparator
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
						comparator
						);
			}

			[[nodiscard]] constexpr static const_iterator begin() noexcept { return std::begin(value); }

			[[nodiscard]] constexpr static const_iterator end() noexcept { return std::end(value); }
		};

		template<typename, typename>
		struct basic_bilateral_fixed_string;

		template<typename T, T... Left, T... Right>
		struct basic_bilateral_fixed_string<basic_fixed_string<T, Left...>, basic_fixed_string<T, Right...>>
		{
			using left_type = basic_fixed_string<T, Left...>;
			using right_type = basic_fixed_string<T, Right...>;

			using value_type = typename left_type::template value_type;
			using size_type = typename left_type::template size_type;

			using const_iterator = typename left_type::template const_iterator;

			constexpr static size_type left_size = left_type::template size;
			constexpr static size_type left_size_no_0 = left_type::template size_no_0;
			constexpr static size_type right_size = right_type::template size;
			constexpr static size_type right_size_no_0 = right_type::template size_no_0;

			[[nodiscard]] constexpr static bool match_left(const value_type* string) noexcept { return left_type::template match(string); }

			[[nodiscard]] constexpr static bool match_right(const value_type* string) noexcept { return right_type::template match(string); }

			/**
			 * @note CharGetter' s parameters are the indices of value
			 * @note CharComparator' s first parameter is container's element 
			 */
			template<template<typename...> typename Container, typename CharGetter, typename CharComparator = std::equal_to<>, typename... AnyOther>
			[[nodiscard]] constexpr static bool match_left(
					const Container<value_type, AnyOther...>& container,
					CharGetter getter,
					CharComparator comparator = CharComparator{}) noexcept(std::is_nothrow_invocable_v<CharGetter, const Container<value_type, AnyOther...>&, size_type> &&
					                                                       std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>) requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				} -> std::convertible_to<bool>;
			} { return left_type::template match(container, getter, comparator); }

			/**
			 * @note CharGetter' s parameters are the indices of value
			 * @note CharComparator' s first parameter is container's element 
			 */
			template<template<typename...> typename Container, typename CharGetter, typename CharComparator = std::equal_to<>, typename... AnyOther>
			[[nodiscard]] constexpr static bool match_right(
					const Container<value_type, AnyOther...>& container,
					CharGetter getter,
					CharComparator comparator = CharComparator{}) noexcept(std::is_nothrow_invocable_v<CharGetter, const Container<value_type, AnyOther...>&, size_type> &&
					                                                       std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>) requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				} -> std::convertible_to<bool>;
			} { return right_type::template match(container, getter, comparator); }

			/**
			 * @note CharGetter' s parameters are the indices of value
			 * @note CharComparator' s first parameter is container's element 
			 */
			template<typename Container, typename CharGetter, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
			[[nodiscard]] constexpr static bool match_left(
					const Container& container,
					CharGetter getter,
					CharComparator comparator = CharComparator{}) noexcept(std::is_nothrow_invocable_v<CharGetter, const Container&, size_type> &&
					                                                       std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>) requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				} -> std::convertible_to<bool>;
			} { return left_type::match(container, getter, comparator); }

			/**
			 * @note CharGetter' s parameters are the indices of value
			 * @note CharComparator' s first parameter is container's element 
			 */
			template<typename Container, typename CharGetter, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
			[[nodiscard]] constexpr static bool match_right(
					const Container& container,
					CharGetter getter,
					CharComparator comparator = CharComparator{}) noexcept(std::is_nothrow_invocable_v<CharGetter, const Container&, size_type> &&
					                                                       std::is_nothrow_invocable_v<CharComparator, decltype(getter(container, std::declval<size_type>())), value_type>) requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					comparator(getter(container, std::declval<size_type>()), std::declval<value_type>())
				} -> std::convertible_to<bool>;
			} { return right_type::match(container, getter, comparator); }

			template<template<typename...> typename Container, typename CharComparator = std::equal_to<>, typename... AnyOther>
			[[nodiscard]] constexpr static bool match_left(
					const Container<value_type, AnyOther...>& container,
					CharComparator comparator = {}) noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(match(
							                                         container,
							                                         [](const auto& c, size_type index) { return c[index]; },
							                                         comparator))) requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					container[std::declval<size_type>()]
				} -> std::same_as<value_type>;
			} { return left_type::template match(container, comparator); }

			template<template<typename...> typename Container, typename CharComparator = std::equal_to<>, typename... AnyOther>
			[[nodiscard]] constexpr static bool match_right(
					const Container<value_type, AnyOther...>& container,
					CharComparator comparator = {}) noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(match(
							                                         container,
							                                         [](const auto& c, size_type index) { return c[index]; },
							                                         comparator))) requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
				{
					container[std::declval<size_type>()]
				} -> std::same_as<value_type>;
			} { return right_type::template match(container, comparator); }

			template<typename Container, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
			[[nodiscard]] constexpr static bool match_left(
					const Container& container,
					CharComparator comparator = {}) noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(match(
							                                         container,
							                                         [](const auto& c, size_type index) { return c[index]; },
							                                         comparator))) requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
			} { return left_type::match(container, comparator); }

			template<typename Container, typename CharComparator = decltype([](const std::decay_t<decltype(std::declval<Container>()[0])> lhs, const value_type rhs) { return static_cast<value_type>(lhs) == rhs; })>
			[[nodiscard]] constexpr static bool match_right(
					const Container& container,
					CharComparator comparator = {}) noexcept(noexcept(container[std::declval<size_type>()]) && noexcept(match(
							                                         container,
							                                         [](const auto& c, size_type index) { return c[index]; },
							                                         comparator))) requires requires
			{
				{
					container.size()
				} -> std::convertible_to<size_type>;
			} { return right_type::match(container, comparator); }

			[[nodiscard]] constexpr static const_iterator left_begin() noexcept { return left_type::template begin(); }

			[[nodiscard]] constexpr static const_iterator left_end() noexcept { return left_type::template end(); }

			[[nodiscard]] constexpr static const_iterator right_begin() noexcept { return right_type::template begin(); }

			[[nodiscard]] constexpr static const_iterator right_end() noexcept { return right_type::template end(); }
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

	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char>
	using fixed_bilateral_string = detail::basic_bilateral_fixed_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, wchar_t>
	using fixed_bilateral_wstring = detail::basic_bilateral_fixed_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char8_t>
	// ReSharper disable once CppInconsistentNaming
	using fixed_bilateral_u8string = detail::basic_bilateral_fixed_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char16_t>
	// ReSharper disable once CppInconsistentNaming
	using fixed_bilateral_u16string = detail::basic_bilateral_fixed_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char32_t>
	// ReSharper disable once CppInconsistentNaming
	using fixed_bilateral_u32string = detail::basic_bilateral_fixed_string<Left, Right>;

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

	#define GAL_UTILS_BILATERAL_FIXED_STRING_TYPE(left_string, right_string) ::gal::utils::fixed_bilateral_string<GAL_UTILS_FIXED_STRING_TYPE(left_string), GAL_UTILS_FIXED_STRING_TYPE(right_string)>
	#define GAL_UTILS_BILATERAL_FIXED_WSTRING_TYPE(left_string, right_string) ::gal::utils::fixed_bilateral_wstring<GAL_UTILS_FIXED_WSTRING_TYPE(left_string), GAL_UTILS_FIXED_WSTRING_TYPE(right_string)>
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_BILATERAL_FIXED_U8STRING_TYPE(left_string, right_string) ::gal::utils::fixed_bilateral_u8string<GAL_UTILS_FIXED_U8STRING_TYPE(left_string), GAL_UTILS_FIXED_U8STRING_TYPE(right_string)>
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_BILATERAL_FIXED_U16STRING_TYPE(left_string, right_string) ::gal::utils::fixed_bilateral_u16string<GAL_UTILS_FIXED_U16STRING_TYPE(left_string), GAL_UTILS_FIXED_U16STRING_TYPE(right_string)>
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_BILATERAL_FIXED_U32STRING_TYPE(left_string, right_string) ::gal::utils::fixed_bilateral_u32string<GAL_UTILS_FIXED_U32STRING_TYPE(left_string), GAL_UTILS_FIXED_U32STRING_TYPE(right_string)>
}

#endif // GAL_UTILS_FIXED_STRING_HPP
