#pragma once

#ifndef GAL_UTILS_TEMPLATE_STRING_HPP
#define GAL_UTILS_TEMPLATE_STRING_HPP

#include <utils/constexpr_string_base.hpp>

namespace gal::utils
{
	namespace template_string_detail
	{
		template<typename T, T... Chars>
		struct basic_template_string : constexpr_string_base<basic_template_string<T, Chars...>, T>
		{
			using value_type = T;
			using size_type = std::size_t;

			using const_iterator = const value_type*;

			constexpr static size_type size = sizeof...(Chars);
			constexpr static value_type value[size]{Chars...};
			constexpr static size_type size_no_0 = size - (value[size - 1] == '\0');// include '\0' ?

			[[nodiscard]] constexpr static const_iterator begin() noexcept { return std::ranges::begin(value); }

			[[nodiscard]] constexpr static const_iterator end() noexcept { return std::ranges::end(value); }
		};

		template<typename, typename>
		struct basic_bilateral_template_string;

		template<typename T, T... Left, T... Right>
		struct basic_bilateral_template_string<basic_template_string<T, Left...>, basic_template_string<T, Right...>>
				: bilateral_constexpr_string_base<basic_bilateral_template_string<basic_template_string<T, Left...>, basic_template_string<T, Right...>>, basic_template_string<T, Left...>, basic_template_string<T, Right...>, typename basic_template_string<T, Left...>::value_type>
		{
			using left_type = basic_template_string<T, Left...>;
			using right_type = basic_template_string<T, Right...>;

			using value_type = typename left_type::template value_type;
			using size_type = typename left_type::template size_type;

			using const_iterator = typename left_type::template const_iterator;

			constexpr static size_type left_size = left_type::template size;
			constexpr static size_type left_size_no_0 = left_type::template size_no_0;
			constexpr static size_type right_size = right_type::template size;
			constexpr static size_type right_size_no_0 = right_type::template size_no_0;

			[[nodiscard]] constexpr static const_iterator left_begin() noexcept { return left_type::template begin(); }

			[[nodiscard]] constexpr static const_iterator left_end() noexcept { return left_type::template end(); }

			[[nodiscard]] constexpr static const_iterator right_begin() noexcept { return right_type::template begin(); }

			[[nodiscard]] constexpr static const_iterator right_end() noexcept { return right_type::template end(); }
		};
	}

	template<char... Chars>
	using template_string = template_string_detail::basic_template_string<char, Chars...>;
	template<wchar_t... Chars>
	using template_wstring = template_string_detail::basic_template_string<wchar_t, Chars...>;
	template<char8_t... Chars>
	// ReSharper disable once CppInconsistentNaming
	using template_u8string = template_string_detail::basic_template_string<char8_t, Chars...>;
	template<char16_t... Chars>
	// ReSharper disable once CppInconsistentNaming
	using template_u16string = template_string_detail::basic_template_string<char16_t, Chars...>;
	template<char32_t... Chars>
	// ReSharper disable once CppInconsistentNaming
	using template_u32string = template_string_detail::basic_template_string<char32_t, Chars...>;

	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char>
	using template_bilateral_string = template_string_detail::basic_bilateral_template_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, wchar_t>
	using template_bilateral_wstring = template_string_detail::basic_bilateral_template_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char8_t>
	// ReSharper disable once CppInconsistentNaming
	using template_bilateral_u8string = template_string_detail::basic_bilateral_template_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char16_t>
	// ReSharper disable once CppInconsistentNaming
	using template_bilateral_u16string = template_string_detail::basic_bilateral_template_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char32_t>
	// ReSharper disable once CppInconsistentNaming
	using template_bilateral_u32string = template_string_detail::basic_bilateral_template_string<Left, Right>;

	#define GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(string_type, string, string_length, begin_index) \
		decltype(                                                                                         \
				[]<std::size_t... Index>(std::index_sequence<Index...>) constexpr noexcept {              \
					return ::gal::utils::string_type<[](std::size_t index) constexpr noexcept           \
					{                                                                                     \
						return (string)[(begin_index) + index];                                                           \
					}                                                                                     \
					(Index)... > {};                                                                      \
				}(std::make_index_sequence<string_length>{}))

	#define GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_TYPE_GENERATOR(string_type, string) \
	GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(string_type, string, sizeof(string) / sizeof((string)[0]), 0)

	#define GAL_UTILS_TEMPLATE_STRING_TYPE(string) GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_TYPE_GENERATOR(template_string, string)
	#define GAL_UTILS_TEMPLATE_WSTRING_TYPE(string) GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_TYPE_GENERATOR(template_wstring, string)
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_TEMPLATE_U8STRING_TYPE(string) GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_TYPE_GENERATOR(template_u8string, string)
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_TEMPLATE_U16STRING_TYPE(string) GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_TYPE_GENERATOR(template_u16string, string)
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_TEMPLATE_U32STRING_TYPE(string) GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_TYPE_GENERATOR(template_u32string, string)

	#define GAL_UTILS_BILATERAL_TEMPLATE_STRING_TYPE(left_string, right_string) ::gal::utils::template_bilateral_string<GAL_UTILS_TEMPLATE_STRING_TYPE(left_string), GAL_UTILS_TEMPLATE_STRING_TYPE(right_string)>
	#define GAL_UTILS_BILATERAL_TEMPLATE_WSTRING_TYPE(left_string, right_string) ::gal::utils::template_bilateral_wstring<GAL_UTILS_TEMPLATE_WSTRING_TYPE(left_string), GAL_UTILS_TEMPLATE_WSTRING_TYPE(right_string)>
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_BILATERAL_TEMPLATE_U8STRING_TYPE(left_string, right_string) ::gal::utils::template_bilateral_u8string<GAL_UTILS_TEMPLATE_U8STRING_TYPE(left_string), GAL_UTILS_TEMPLATE_U8STRING_TYPE(right_string)>
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_BILATERAL_TEMPLATE_U16STRING_TYPE(left_string, right_string) ::gal::utils::template_bilateral_u16string<GAL_UTILS_TEMPLATE_U16STRING_TYPE(left_string), GAL_UTILS_TEMPLATE_U16STRING_TYPE(right_string)>
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_BILATERAL_TEMPLATE_U32STRING_TYPE(left_string, right_string) ::gal::utils::template_bilateral_u32string<GAL_UTILS_TEMPLATE_U32STRING_TYPE(left_string), GAL_UTILS_TEMPLATE_U32STRING_TYPE(right_string)>

	#define GAL_UTILS_SYMMETRY_TEMPLATE_STRING_TYPE(string) \
		::gal::utils::template_bilateral_string<\
		GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(template_string, string, sizeof(string) / sizeof((string)[0]) / 2, 0), \
		GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(template_string, string, sizeof(string) / sizeof((string)[0]) / 2, sizeof(string) / sizeof((string)[0]) / 2)>
	#define GAL_UTILS_SYMMETRY_TEMPLATE_WSTRING_TYPE(string)  \
		::gal::utils::template_bilateral_wstring<                                         \
			GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(template_wstring, string, sizeof(string) / sizeof((string)[0]) / 2, 0), \
			GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(template_wstring, string, sizeof(string) / sizeof((string)[0]) / 2, sizeof(string) / sizeof((string)[0]) / 2)>
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_SYMMETRY_TEMPLATE_U8STRING_TYPE(string)                                                                                      \
		::gal::utils::template_bilateral_u8string<                                         \
			GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(template_u8string, string, sizeof(string) / sizeof((string)[0]) / 2, 0), \
			GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(template_u8string, string, sizeof(string) / sizeof((string)[0]) / 2, sizeof(string) / sizeof((string)[0]) / 2)>
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_SYMMETRY_TEMPLATE_U16STRING_TYPE(string)  \
		::gal::utils::template_bilateral_u16string<                                         \
			GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(template_u16string, string, sizeof(string) / sizeof((string)[0]) / 2, 0), \
			GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(template_u16string, string, sizeof(string) / sizeof((string)[0]) / 2, sizeof(string) / sizeof((string)[0]) / 2)>
	// ReSharper disable once CppInconsistentNaming
	#define GAL_UTILS_SYMMETRY_TEMPLATE_U32STRING_TYPE(string)  \
		::gal::utils::template_bilateral_u32string<                                         \
			GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(template_u32string, string, sizeof(string) / sizeof((string)[0]) / 2, 0), \
			GAL_UTILS_DO_NOT_USE_TEMPLATE_STRING_INNER_TYPE_GENERATOR(template_u32string, string, sizeof(string) / sizeof((string)[0]) / 2, sizeof(string) / sizeof((string)[0]) / 2)>
}

#endif // GAL_UTILS_TEMPLATE_STRING_HPP
