#pragma once

#ifndef GAL_UTILS_ENUM_UTILS_HPP
#define GAL_UTILS_ENUM_UTILS_HPP

#include <type_traits>

namespace gal::utils
{
	struct enum_flag_processor_set
	{
		template<typename Value, typename Enum>
			requires std::is_scalar_v<Value> && std::is_enum_v<Enum> && std::is_convertible_v<std::underlying_type_t<Enum>, Value>
		constexpr void operator()(Value& v, const Enum e) const noexcept { v |= static_cast<Value>(e); }
	};

	struct enum_flag_processor_unset
	{
		template<typename Value, typename Enum>
			requires std::is_scalar_v<Value> && std::is_enum_v<Enum> && std::is_convertible_v<std::underlying_type_t<Enum>, Value>
		constexpr void operator()(Value& v, const Enum e) const noexcept { v &= ~static_cast<Value>(e); }
	};

	struct enum_flag_processor_check
	{
		template<typename Value, typename Enum>
			requires std::is_scalar_v<Value> && std::is_enum_v<Enum> && std::is_convertible_v<std::underlying_type_t<Enum>, Value>
		constexpr bool operator()(const Value& v, const Enum e) const noexcept { return v & static_cast<Value>(e); }
	};

	struct enum_flag_processor_flip
	{
		template<typename Value, typename Enum>
			requires std::is_scalar_v<Value> && std::is_enum_v<Enum> && std::is_convertible_v<std::underlying_type_t<Enum>, Value>
		constexpr void operator()(Value& v, const Enum e) const noexcept
		{
			if (enum_flag_processor_check{}(v, e))
			{
				// set
				enum_flag_processor_unset{}(v, e);
			}
			else
			{
				// not set
				enum_flag_processor_set{}(v, e);
			}
		}
	};

	// struct enum_flag_processor_compare
	// {
	// 	template<typename Value, typename Enum>
	// 		requires std::is_scalar_v<Value> && std::is_enum_v<Enum> && std::is_convertible_v<std::underlying_type_t<Enum>, Value>
	// 	constexpr bool operator()(const Value& v, const Enum e) noexcept { return v <=> static_cast<Value>(e); }
	// };

	template<bool Conjunction, typename Operator, typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...) &&
		         (std::is_invocable_v<Operator, Value, Enums> && ...)
	constexpr bool enum_flag_process_check(Operator o, Value v, Enums ... enums) noexcept
	{
		if constexpr (Conjunction)
		{
			return ([&v, o](const auto e) { return o(v, e); }(enums) &&
				...);
		}
		else
		{
			return ([&v, o](const auto e) { return o(v, e); }(enums) ||
				...);
		}
	}

	template<typename Operator, typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...) &&
		         (std::is_invocable_v<Operator, Value&, Enums> && ...)
	constexpr void enum_flag_process_set(Operator o, Value& v, Enums ... enums) noexcept
	{
		([&v, o](const auto e) { o(v, e); }(enums),
			...);
	}

	template<typename Operator, typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...) &&
		         (std::is_invocable_v<Operator, Value&, Enums> && ...)
	constexpr Value enum_flag_process_ret(Operator o, Value v, Enums ... enums) noexcept
	{
		Value flag = v;
		enum_flag_process_set(o, flag, enums...);
		return flag;
	}

	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr void set_enum_flag_set(Value& v, Enums ... enums) noexcept { enum_flag_process_set(enum_flag_processor_set{}, v, enums...); }

	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr Value set_enum_flag_ret(Value v, Enums ... enums) noexcept { return enum_flag_process_ret(enum_flag_processor_set{}, v, enums...); }

	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr void set_enum_flag_set(Enum& e, std::convertible_to<std::underlying_type_t<Enum>> auto ... enums) noexcept { set_enum_flag_set(static_cast<std::underlying_type_t<Enum>&>(e), enums...); }

	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr Enum set_enum_flag_ret(Enum e, Enums ... enums) noexcept { return static_cast<Enum>(set_enum_flag_ret(static_cast<std::underlying_type_t<Enum>>(e), enums...)); }

	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr void unset_enum_flag_set(Value& v, Enums ... enums) noexcept { enum_flag_process_set(enum_flag_processor_unset{}, v, enums...); }

	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr Value unset_enum_flag_ret(Value& v, Enums ... enums) noexcept { return enum_flag_process_ret(enum_flag_processor_unset{}, v, enums...); }

	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr void unset_enum_flag_set(Enum& e, Enums ... enums) noexcept { unset_enum_flag_set(static_cast<std::underlying_type_t<Enum>&>(e), enums...); }

	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr Enum unset_enum_flag_ret(Enum e, Enums ... enums) noexcept { return static_cast<Enum>(unset_enum_flag_ret(static_cast<std::underlying_type_t<Enum>>(e), enums...)); }

	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr bool check_all_enum_flag(Value v, Enums ... enums) noexcept { return enum_flag_process_check<true>(enum_flag_processor_check{}, v, enums...); }

	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr bool check_any_enum_flag(Value v, Enums ... enums) noexcept { return enum_flag_process_check<false>(enum_flag_processor_check{}, v, enums...); }

	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr bool check_all_enum_flag(Enum e, Enums ... enums) noexcept { return check_all_enum_flag(static_cast<std::underlying_type_t<Enum>>(e), enums...); }

	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr bool check_any_enum_flag(Enum e, Enums ... enums) noexcept { return check_any_enum_flag(static_cast<std::underlying_type_t<Enum>>(e), enums...); }

	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr void flip_enum_flag_set(Value& v, Enums ... enums) noexcept { enum_flag_process_set<false, false>(enum_flag_processor_flip{}, v, enums...); }

	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr Value flip_enum_flag_ret(Value& v, Enums ... enums) noexcept { return enum_flag_process_ret<false, false>(enum_flag_processor_flip{}, v, enums...); }

	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr void flip_enum_flag_set(Enum& e, Enums ... enums) noexcept { flip_enum_flag_set(static_cast<std::underlying_type_t<Enum>&>(e), enums...); }

	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr Enum flip_enum_flag_ret(Enum e, Enums ... enums) noexcept { return static_cast<Enum>(flip_enum_flag_ret(static_cast<std::underlying_type_t<Enum>>(e), enums...)); }

	template<typename T, typename... Args>
		requires std::is_enum_v<T> && ((std::is_convertible_v<Args, T> || std::is_convertible_v<Args, std::underlying_type_t<T>>) && ...)
	constexpr bool is_all_enum_of(T current_enum, Args ... requires_enums) noexcept { return check_all_enum_flag(current_enum, requires_enums...); }

	template<typename T, typename... Args>
		requires std::is_enum_v<T> && ((std::is_convertible_v<Args, T> || std::is_convertible_v<Args, std::underlying_type_t<T>>) && ...)
	constexpr bool is_any_enum_of(T current_enum, Args ... requires_enums) noexcept { return check_any_enum_flag(current_enum, requires_enums...); }

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
}

#endif // GAL_UTILS_ENUM_UTILS_HPP
