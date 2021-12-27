#pragma once

#ifndef GAL_LANG_UTILS_STRING_UTILS_HPP
#define GAL_LANG_UTILS_STRING_UTILS_HPP

#include<string>
#include<string_view>
#include <range/v3/view.hpp>

#include<utils/concept.hpp>

namespace gal::utils
{
	template<typename String, template<typename> typename Container>
		requires is_any_type_of_v<String, std::basic_string<typename String::value_type, typename String::traits_type>, std::basic_string_view<typename String::value_type, typename String::traits_type>>
	constexpr std::basic_string<typename String::value_type, typename String::traits_type> join(
			const Container<String>& segments,
			std::basic_string_view<typename String::value_type, typename String::traits_type> delimiter
			)
	{
		using return_type = std::basic_string<typename String::value_type, typename String::traits_type>;

		if (std::empty(segments)) { return return_type{}; }

		return segments | ranges::views::join(delimiter) | ranges::to<return_type>();
	}

	/**
	 * @param delimiter String's delimiter
	 * @param string String
	 * @param inserter An iterator used to insert elements into the target container, usually back_inserter_iterator
	 *
	 * Putting `delimiter` in the first parameter and using std::type_identity_t for the remaining parameters allows us to call through
	 * split("delimiter"sv, str, inserter) without the need to specify template parameters.
	 */
	template<typename String, typename Inserter>
		requires is_any_type_of_v<String, std::basic_string_view<typename String::value_type, typename String::traits_type>> &&
		         std::is_convertible_v<String, typename Inserter::container_type::value_type> &&
		         requires(String string, Inserter inserter)
		         {
			         inserter = string;
		         }
	constexpr void split(String delimiter, std::type_identity_t<String> string, Inserter inserter)
	{
		(void)inserter;

		while (not string.empty())
		{
			auto index = string.find(delimiter);
			if (index == String::npos)
			{
				inserter = string;
				break;
			}
			inserter = string.substr(0, index);
			string.remove_prefix(index + 1);
		}
	}

	/**
	 * @param delimiter String's delimiter
	 * @param string String
	 * @return A view containing all the split strings, can be taken out by iteration
	 *
	 * Putting `delimiter` in the first parameter and using std::type_identity_t for the remaining parameters allows us to call through
	 * split("delimiter"sv, str) without the need to specify template parameters.
	 */
	template<typename String>
		requires is_any_type_of_v<String, std::basic_string_view<typename String::value_type, typename String::traits_type>>
	constexpr auto split(String delimiter, std::type_identity_t<String> string) { return string | ranges::views::split(delimiter) | ranges::views::transform([](auto&& range) { return String(&*range.begin(), ranges::distance(range)); }); }

	/**
	 * @param delimiter String's delimiter
	 * @param string String
	 * @param container Container to be inserted, we will use container::push_back to insert elements
	 *
	 * Putting `delimiter` in the first parameter and using std::type_identity_t for the remaining parameters allows us to call through
	 * split("delimiter"sv, str, container) without the need to specify template parameters.
	 */
	template<typename String, typename Container>
		requires is_any_type_of_v<String, std::basic_string_view<typename String::value_type, typename String::traits_type>> &&
		         std::is_convertible_v<String, typename Container::value_type> &&
		         requires(String string, Container container)
		         {
			         container.push_back(string);
		         }
	constexpr void split(String delimiter, std::type_identity_t<String> string, Container& container) { for (auto&& each: split(delimiter, string)) { container.push_back(std::move(each)); } }

	constexpr bool is_whitespace(const char c) noexcept { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f'; }

	constexpr bool is_new_line(const char c) noexcept { return c == '\n' || c == '\r'; }

	constexpr unsigned to_alpha(const char c) noexcept { return static_cast<unsigned>((c | ' ') - 'a') + 10; }

	constexpr bool is_alpha(const char c) noexcept { return to_alpha(c) < 26; }

	constexpr unsigned to_digit(const char c) noexcept { return static_cast<unsigned>(c - '0'); }

	constexpr bool is_digit(const char c) noexcept { return to_digit(c) < 10; }

	constexpr bool is_hex_digit(const char c) noexcept { return to_digit(c) < 10 || to_alpha(c) < 16; }

	constexpr char take_escape(const char c)
	{
		switch (c)
		{
			case 'a': { return '\a'; }
			case 'b': { return '\b'; }
			case 'f': { return '\f'; }
			case 'n': { return '\n'; }
			case 'r': { return '\r'; }
			case 't': { return '\t'; }
			case 'v': { return '\v'; }
			default: { return c; }
		}
	}

	constexpr std::size_t to_utf8(char* data, std::uint32_t codepoint)
	{
		// U+0000..U+007F
		if (codepoint < 0x80)
		{
			data[0] = static_cast<char>(codepoint);
			return 1;
		}
		// U+0080..U+07FF
		if (codepoint < 0x800)
		{
			data[0] = static_cast<char>(0xC0 | (codepoint >> 6));
			data[1] = static_cast<char>(0x80 | (codepoint & 0x3F));
			return 2;
		}
		// U+0800..U+FFFF
		if (codepoint < 0x10000)
		{
			data[0] = static_cast<char>(0xE0 | (codepoint >> 12));
			data[1] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
			data[2] = static_cast<char>(0x80 | (codepoint & 0x3F));
			return 3;
		}
		// U+10000..U+10FFFF
		if (codepoint < 0x110000)
		{
			data[0] = static_cast<char>(0xF0 | (codepoint >> 18));
			data[1] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
			data[2] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
			data[3] = static_cast<char>(0x80 | (codepoint & 0x3F));
			return 4;
		}
		return 0;
	}
}

#endif // GAL_LANG_UTILS_STRING_UTILS_HPP
