#pragma once

#ifndef GAL_UTILS_FIXED_STRING_HPP
#define GAL_UTILS_FIXED_STRING_HPP

#include <utils/constexpr_string_base.hpp>

namespace gal::utils
{
	namespace fixed_string_detail
	{
		template<typename T, std::size_t N>
		struct basic_fixed_string : constexpr_string_base<basic_fixed_string<T, N>, T>
		{
			using value_type = T;
			using size_type = std::size_t;

			using const_iterator = const value_type*;

			constexpr static size_type size = N;
			constexpr static size_type size_no_0 = size;
			constexpr static value_type value[size]{};
		};

		// todo: Why not just use string_view directly?
		template<typename T>
		struct basic_fixed_string_view : constexpr_string_base<basic_fixed_string_view<T>, T>
		{
			using value_type = T;
			using size_type = std::size_t;
			using const_iterator = const value_type*;

			size_type size;
			value_type* value = nullptr;

			template<std::size_t N>
			explicit constexpr basic_fixed_string_view(const char (&string)[N]) noexcept
				: size{N} { std::ranges::copy(string, value); }

			[[nodiscard]] constexpr size_type size_no_0() const noexcept { return size - 1; }

			[[nodiscard]] const_iterator begin() const noexcept { return value; }

			[[nodiscard]] constexpr const_iterator end() const noexcept { return value + size; }
		};

		template<typename, typename>
		struct basic_bilateral_fixed_string;

		template<typename T, std::size_t Left, std::size_t Right>
		struct basic_bilateral_fixed_string<basic_fixed_string<T, Left>, basic_fixed_string<T, Right>>
				: bilateral_constexpr_string_base<basic_bilateral_fixed_string<basic_fixed_string<T, Left>, basic_fixed_string<T, Right>>, basic_fixed_string<T, Left>, basic_fixed_string<T, Right>>
		{
			using left_type = basic_fixed_string<T, Left>;
			using right_type = basic_fixed_string<T, Right>;

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

		template<typename, typename>
		struct basic_bilateral_fixed_string_view;

		template<typename T>
		struct basic_bilateral_fixed_string_view<basic_fixed_string_view<T>, basic_fixed_string_view<T>>
				: bilateral_constexpr_string_base<basic_bilateral_fixed_string_view<basic_fixed_string_view<T>, basic_fixed_string_view<T>>, basic_fixed_string_view<T>, basic_fixed_string_view<T>>
		{
			using left_type = basic_fixed_string_view<T>;
			using right_type = basic_fixed_string_view<T>;

			using value_type = typename left_type::template value_type;
			using size_type = typename left_type::template size_type;
			using const_iterator = typename left_type::template const_iterator;

			left_type left_value;
			right_type right_value;

			[[nodiscard]] constexpr const_iterator left_begin() noexcept { return left_value.begin(); }

			[[nodiscard]] constexpr const_iterator left_end() noexcept { return left_value.end(); }

			[[nodiscard]] constexpr const_iterator right_begin() noexcept { return right_value.begin(); }

			[[nodiscard]] constexpr const_iterator right_end() noexcept { return right_value.end(); }
		};
	}

	template<std::size_t N>
	using fixed_string = fixed_string_detail::basic_fixed_string<char, N>;
	template<std::size_t N>
	using fixed_wstring = fixed_string_detail::basic_fixed_string<wchar_t, N>;
	template<std::size_t N>
	// ReSharper disable once CppInconsistentNaming
	using fixed_u8string = fixed_string_detail::basic_fixed_string<char8_t, N>;
	template<std::size_t N>
	// ReSharper disable once CppInconsistentNaming
	using fixed_u16string = fixed_string_detail::basic_fixed_string<char16_t, N>;
	template<std::size_t N>
	// ReSharper disable once CppInconsistentNaming
	using fixed_u32string = fixed_string_detail::basic_fixed_string<char32_t, N>;

	using fixed_string_view = fixed_string_detail::basic_fixed_string_view<char>;
	using fixed_wstring_view = fixed_string_detail::basic_fixed_string_view<wchar_t>;
	// ReSharper disable once CppInconsistentNaming
	using fixed_u8string_view = fixed_string_detail::basic_fixed_string_view<char8_t>;
	// ReSharper disable once CppInconsistentNaming
	using fixed_u16string_view = fixed_string_detail::basic_fixed_string_view<char16_t>;
	// ReSharper disable once CppInconsistentNaming
	using fixed_u32string_view = fixed_string_detail::basic_fixed_string_view<char32_t>;

	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char>
	using fixed_bilateral_string = fixed_string_detail::basic_bilateral_fixed_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, wchar_t>
	using fixed_bilateral_wstring = fixed_string_detail::basic_bilateral_fixed_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char8_t>
	// ReSharper disable once CppInconsistentNaming
	using fixed_bilateral_u8string = fixed_string_detail::basic_bilateral_fixed_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char16_t>
	// ReSharper disable once CppInconsistentNaming
	using fixed_bilateral_u16string = fixed_string_detail::basic_bilateral_fixed_string<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char32_t>
	// ReSharper disable once CppInconsistentNaming
	using fixed_bilateral_u32string = fixed_string_detail::basic_bilateral_fixed_string<Left, Right>;

	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char>
	using fixed_bilateral_string_view = fixed_string_detail::basic_bilateral_fixed_string_view<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, wchar_t>
	using fixed_bilateral_wstring_view = fixed_string_detail::basic_bilateral_fixed_string_view<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char8_t>
	// ReSharper disable once CppInconsistentNaming
	using fixed_bilateral_u8string_view = fixed_string_detail::basic_bilateral_fixed_string_view<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char16_t>
	// ReSharper disable once CppInconsistentNaming
	using fixed_bilateral_u16string_view = fixed_string_detail::basic_bilateral_fixed_string_view<Left, Right>;
	template<typename Left, typename Right>
		requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char32_t>
	// ReSharper disable once CppInconsistentNaming
	using fixed_bilateral_u32string_view = fixed_string_detail::basic_bilateral_fixed_string_view<Left, Right>;

	// todo: generator?
}

#endif // GAL_UTILS_FIXED_STRING_HPP
