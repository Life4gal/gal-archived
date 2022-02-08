#pragma once

#ifndef GAL_UTILS_FIXED_STRING_HPP
#define GAL_UTILS_FIXED_STRING_HPP

namespace gal::utils
{
	template<char... Chars>
	struct fixed_string
	{
		constexpr static std::size_t size = sizeof...(Chars);// include '\0'
		constexpr static std::size_t size_no_0 = size - 1;
		constexpr static char value[size]{Chars...};

		constexpr static bool match(const char* string) noexcept
		{
			return std::char_traits<char>::length(string) == size_no_0 &&
			       std::char_traits<char>::compare(value, string, size_no_0) == 0;
		}
	};

	#define GAL_UTILS_FIXED_STRING_TYPE(string)                                                              \
	decltype(                                                                            \
			[]<std::size_t... Index>(std::index_sequence<Index...>) constexpr noexcept \
			{ \
				return fixed_string<[](std::size_t index) constexpr noexcept           \
				{                                                                        \
					return (string)[index];                                              \
				}                                                                        \
				(Index)...>{};                                                         \
			}(std::make_index_sequence<sizeof(string)>()))
}

#endif // GAL_UTILS_FIXED_STRING_HPP
