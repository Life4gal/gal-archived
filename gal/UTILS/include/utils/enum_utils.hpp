#pragma once

#ifndef GAL_LANG_UTILS_ENUM_UTILS_HPP
#define GAL_LANG_UTILS_ENUM_UTILS_HPP

namespace gal::utils
{
	template<typename T, typename... Args>
		requires std::is_enum_v<T> && ((std::is_convertible_v<Args, T> || std::is_convertible_v<Args, std::underlying_type_t<T>>) && ...)
	constexpr bool is_any_enum_of(T current_enum, Args ... requires_enums) noexcept
	{
		return ([current = static_cast<std::underlying_type_t<T>>(current_enum)](auto requires_enum) { return current == static_cast<std::underlying_type_t<T>>(requires_enum); }(requires_enums) ||
			...);
	}

	template<bool Opened = true, bool Closed = true, typename T, typename U>
		requires std::is_enum_v<T> && (std::is_convertible_v<U, T> || std::is_convertible_v<U, std::underlying_type_t<T>>)
	constexpr bool is_enum_between_of(T current_enum, U enum_begin, std::type_identity_t<U> enum_end) noexcept
	{
		using underlying_type = std::underlying_type_t<T>;

		const auto current = static_cast<underlying_type>(current_enum);

		underlying_type begin{};
		underlying_type end{};
		if constexpr (Opened) { begin = static_cast<underlying_type>(enum_begin); }
		else { begin = static_cast<underlying_type>(enum_begin) + 1; }
		if constexpr (Closed) { end = static_cast<underlying_type>(enum_end); }
		else { end = static_cast<underlying_type>(enum_end) - 1; }

		return begin <= current && current <= end;
	}
}

#endif // GAL_LANG_UTILS_ENUM_UTILS_HPP
