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

		[[nodiscard]] constexpr static bool match(const char* string) noexcept
		{
			return std::char_traits<char>::length(string) == size_no_0 &&
			       std::char_traits<char>::compare(value, string, size_no_0) == 0;
		}

		template<
			template<typename> typename Container,
			char (Container<char>::*CharGetter)(std::size_t) = &Container<char>::operator[]
		>
			requires requires(Container<char> container)
			{
				{
					container.size()
				} -> std::same_as<std::size_t>;
			}
		[[nodiscard]] constexpr static bool match(const Container<char>& container)
		{
			return container.size() == size_no_0 &&
			       [&]<std::size_t... Index>(std::index_sequence<Index...>) { return ((value[Index] == container.CharGetter(Index)) && ...); }(std::make_index_sequence<size_no_0>{});
		}

		// CharGetter' s parameters are indices
		template<typename CharGetter, typename CharCompare = std::equal_to<>>
			requires std::is_invocable_r_v<char, CharGetter, std::size_t>
		[[nodiscard]] constexpr static bool match(CharGetter getter)
		noexcept(
			std::is_nothrow_invocable_r_v<char, CharGetter, std::size_t> &&
			std::is_nothrow_invocable_r_v<bool, CharCompare, char, char>
		)
		{
			CharCompare compare{};
			return [&]<std::size_t... Index>(std::index_sequence<Index...>) { return ((compare(value[Index], getter(Index))) && ...); }(std::make_index_sequence<size_no_0>{});
		}

		template<typename CharGetter, typename CharCompare = std::equal_to<>>
			requires std::is_invocable_r_v<char, CharGetter>
		[[nodiscard]] constexpr static bool match(CharGetter getter) noexcept(
			std::is_nothrow_invocable_r_v<char, CharGetter, std::size_t> &&
			std::is_nothrow_invocable_r_v<bool, CharCompare, char, char>)
		{
			CharCompare compare{};
			return [&]<std::size_t... Index>(std::index_sequence<Index...>) { return ((compare(value[Index], getter())) && ...); }(std::make_index_sequence<size_no_0>{});
		}

		[[nodiscard]] constexpr static const char* begin() noexcept { return std::begin(value); }

		[[nodiscard]] constexpr static const char* end() noexcept { return std::end(value); }
	};

	#define GAL_UTILS_FIXED_STRING_TYPE(string)                                                              \
	decltype(                                                                            \
			[]<std::size_t... Index>(std::index_sequence<Index...>) constexpr noexcept \
			{ \
				return ::gal::utils::fixed_string<[](std::size_t index) constexpr noexcept           \
				{                                                                        \
					return (string)[index];                                              \
				}                                                                        \
				(Index)...>{};                                                         \
			}(std::make_index_sequence<sizeof(string)>()))
}

#endif // GAL_UTILS_FIXED_STRING_HPP
