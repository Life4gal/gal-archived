#pragma once

#ifndef GAL_UTILS_ENUM_UTILS_HPP
#define GAL_UTILS_ENUM_UTILS_HPP

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

	template<typename Operator, typename Enum, typename Value>
		requires std::is_enum_v<Enum> && std::is_scalar_v<Value> &&
		         std::is_convertible_v<std::underlying_type_t<Enum>, Value> &&
		         std::is_convertible_v<Value, std::underlying_type_t<Enum>> &&
		         std::is_invocable_r_v<Value, Operator, Value, Value>
	constexpr decltype(auto) invoke_enum_operator(Enum e, Value v, Operator o) noexcept(std::is_nothrow_invocable_r_v<Value, Operator, Value, Value>) { return o(static_cast<Value>(static_cast<std::underlying_type_t<Enum>>(e)), v); }

	template<typename Operator, typename Value, typename Enum>
		requires std::is_scalar_v<Value> && std::is_enum_v<Enum> &&
		         std::is_convertible_v<std::underlying_type_t<Enum>, Value> &&
		         std::is_convertible_v<Value, std::underlying_type_t<Enum>> &&
		         std::is_invocable_r_v<Value, Operator, Value, Value>
	constexpr decltype(auto) invoke_enum_operator(Value v, Enum e, Operator o) noexcept(std::is_nothrow_invocable_r_v<Value, Operator, Value, Value>) { return o(v, static_cast<Value>(static_cast<std::underlying_type_t<Enum>>(e))); }

	template<bool All = true, typename Value, typename... Enums>
		requires std::is_scalar_v<Value> && (std::is_enum_v<Enums> && ...) && (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr bool is_enum_flag_contains(Value v, Enums ... enums) noexcept
	{
		if constexpr (All)
		{
			return (invoke_enum_operator(
						v,
						enums,
						[](const Value lhs, const Value rhs) constexpr noexcept { return (lhs & rhs) != 0; }) &&
				...);
		}
		else
		{
			return (invoke_enum_operator(
						v,
						enums,
						[](const Value lhs, const Value rhs) constexpr noexcept { return (lhs & rhs) != 0; }) ||
				...);
		}
	}

	struct set_enum_flag_type_set
	{
		template<typename Value, typename Enum>
			requires std::is_scalar_v<Value> && std::is_enum_v<Enum> && std::is_convertible_v<std::underlying_type_t<Enum>, Value>
		constexpr void operator()(Value& v, const Enum e) const noexcept { v |= static_cast<Value>(e); }
	};

	struct set_enum_flag_type_unset
	{
		template<typename Value, typename Enum>
			requires std::is_scalar_v<Value> && std::is_enum_v<Enum> && std::is_convertible_v<std::underlying_type_t<Enum>, Value>
		constexpr void operator()(Value& v, const Enum e) const noexcept { v &= ~static_cast<Value>(e); }
	};

	template<typename Operator, typename Value, typename... Enums>
		requires
		std::is_scalar_v<Value> &&
		(std::is_enum_v<Enums> && ...) &&
		(std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...) &&
		(std::is_invocable_v<Operator, Value&, Enums> && ...)
	constexpr void set_enum_flag(Operator o, Value& v, Enums ... enums) noexcept
	{
		return ([&v, o](const auto e) { o(v, e); }(enums),
			...);
	}

	template<typename Operator, typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...) &&
		         (std::is_invocable_v<Operator, Value&, Enums> && ...)
	constexpr Value set_new_enum_flag(Operator o, const Value& v, Enums ... enums) noexcept
	{
		Value flag = v;
		set_enum_flag(o, flag, enums...);
		return flag;
	}
}

#endif // GAL_UTILS_ENUM_UTILS_HPP
