#pragma once

#ifndef GAL_LANG_UTILS_STRING_UTILS_HPP
#define GAL_LANG_UTILS_STRING_UTILS_HPP

#include<string>
#include<string_view>
#include <ranges>

#include<utils/concept.hpp>

namespace gal
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

		return_type ret{};

		// for (const auto& _: segments | std::views::transform([&](const auto& str) { return ret.append(str).append(delimiter); }) |
		// 							std::views::take(segments.size() - 1)) { (void)_; }
		//
		for (const auto& str : segments | std::views::take(segments.size() - 1)) { ret.append(str).append(delimiter); }

		return ret.append(*std::prev(segments.end()));
	}

	/**
	 * @brief Putting `delimiter` in the first parameter and using std::type_identity_t for the remaining parameters allows us to call through
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
}

#endif // GAL_LANG_UTILS_STRING_UTILS_HPP
