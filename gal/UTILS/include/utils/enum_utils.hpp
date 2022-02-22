#pragma once

#ifndef GAL_UTILS_ENUM_UTILS_HPP
#define GAL_UTILS_ENUM_UTILS_HPP

#include <type_traits>

namespace gal::utils
{
	namespace detail
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

		struct enum_flag_processor_filter
		{
			template<typename Value, typename Enum>
				requires std::is_scalar_v<Value> && std::is_enum_v<Enum> && std::is_convertible_v<std::underlying_type_t<Enum>, Value>
			constexpr void operator()(Value& v, const Enum e) const noexcept { v &= static_cast<Value>(e); }
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
	}

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 * }
	 *
	 * std::uint32_t flag = 0;
	 * set_enum_flag_set(flag, e::a, e::c);
	 * assert(flag == 0x0001 | 0x0100);
	 *
	 * @endcode
	 */
	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr void set_enum_flag_set(Value& v, Enums ... enums) noexcept { enum_flag_process_set(detail::enum_flag_processor_set{}, v, enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 * }
	 *
	 * constexpr std::uint32_t flag = 0x0001;
	 * static_assert(set_enum_flag_ret(flag, a::b, a::d) == 0x0001 | 0x0010 | 0x1000);
	 *
	 * @endcode
	 */
	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr Value set_enum_flag_ret(Value v, Enums ... enums) noexcept { return enum_flag_process_ret(detail::enum_flag_processor_set{}, v, enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * e e1 = e::a;
	 * set_enum_flag_set(e1, a::c, a::d)
	 * assert(e1 == e::e);
	 *
	 * @endcode
	 */
	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr void set_enum_flag_set(Enum& e, std::convertible_to<std::underlying_type_t<Enum>> auto ... enums) noexcept { set_enum_flag_set(static_cast<std::underlying_type_t<Enum>&>(e), enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * static_assert(set_enum_flag_ret(e::a, e::c, e::d) == e::e);
	 *
	 * @endcode
	 */
	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr Enum set_enum_flag_ret(Enum e, Enums ... enums) noexcept { return static_cast<Enum>(set_enum_flag_ret(static_cast<std::underlying_type_t<Enum>>(e), enums...)); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * std::uint32_t e1 = 0x0001 | 0x0100 | 0x0001;
	 * unset_enum_flag_set(e1, e::c, e::d);
	 * assert(e1 == 0x0001);
	 *
	 * @endcode
	 */
	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr void unset_enum_flag_set(Value& v, Enums ... enums) noexcept { enum_flag_process_set(detail::enum_flag_processor_unset{}, v, enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * static_assert(unset_enum_flag_ret(0x0001 | 0x0100 | 0x0001, e::c, e::d) == 0x0001);
	 *
	 * @endcode
	 */
	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr Value unset_enum_flag_ret(Value& v, Enums ... enums) noexcept { return enum_flag_process_ret(detail::enum_flag_processor_unset{}, v, enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * e e1 = e::e;
	 * unset_enum_flag_set(e1, e::c, e::d);
	 * assert(e1 == e::a);
	 *
	 * @endcode
	 */
	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr void unset_enum_flag_set(Enum& e, Enums ... enums) noexcept { unset_enum_flag_set(static_cast<std::underlying_type_t<Enum>&>(e), enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * e e1 = e::e;
	 * unset_enum_flag_ret(e1, e::c, e::d);
	 * assert(e1 == e::a);
	 *
	 * @endcode
	 */
	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr Enum unset_enum_flag_ret(Enum e, Enums ... enums) noexcept { return static_cast<Enum>(unset_enum_flag_ret(static_cast<std::underlying_type_t<Enum>>(e), enums...)); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 *	f = a | b | d,
	 * }
	 *
	 * std::uint32_t flag = 0x0001 | 0x0010 | 0x0100 | 0x1000;
	 * filter_enum_flag_set(flag, e::e, e::f);
	 * assert(flag == 0x0001 | 0x1000);
	 *
	 * @endcode
	 */
	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr void filter_enum_flag_set(Value v, Enums ... enums) noexcept { return enum_flag_process_set(detail::enum_flag_processor_filter{}, v, enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 *	f = a | b | d,
	 * }
	 *
	 * constexpr std::uint32_t flag = 0x0001 | 0x0010 | 0x0100 | 0x1000;
	 * static_assert(filter_enum_flag_ret(flag, e::e, e::f) == 0x0001 | 0x1000);
	 *
	 * @endcode
	 */
	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr Value filter_enum_flag_ret(Value v, Enums ... enums) noexcept { return enum_flag_process_ret(detail::enum_flag_processor_filter{}, v, enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * constexpr std::uint32_t flag = 0x0001 | 0x0100 | 0x1000;
	 * static_assert(check_all_enum_flag(flag));
	 * static_assert(check_all_enum_flag(flag, e::c, e::d));
	 * static_assert(not check_all_enum_flag(flag, e::b, e::c, e::d));
	 *
	 * @endcode
	 */
	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr bool check_all_enum_flag(Value v, Enums ... enums) noexcept { return enum_flag_process_check<true>(detail::enum_flag_processor_check{}, v, enums...); }

	/**
	* @code
	*
	* enum class e : std::uint32_t
	* {
	*	a = 0x0001,
	*	b = 0x0010,
	*	c = 0x0100,
	*	d = 0x1000,
	*	e = a | c | d,
	* }
	*
	* constexpr e e1 = e::e;
	* static_assert(check_all_enum_flag(e1));
	* static_assert(check_all_enum_flag(e1, e::c, e::d));
	* static_assert(not check_all_enum_flag(e1, e::b, e::c, e::d));
	*
	* @endcode
	*/
	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr bool check_all_enum_flag(Enum e, Enums ... enums) noexcept { return check_all_enum_flag(static_cast<std::underlying_type_t<Enum>>(e), enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * constexpr std::uint32_t flag = 0x0001 | 0x0100 | 0x1000;
	 * static_assert(check_any_enum_flag(flag));
	 * static_assert(not check_any_enum_flag(flag, e::b));
	 * static_assert(check_any_enum_flag(flag, e::b, e::c, e::d));
	 * static_assert(check_any_enum_flag(flag, e::b, e::c, e::d));
	 *
	 * @endcode
	 */
	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr bool check_any_enum_flag(Value v, Enums ... enums) noexcept { return enum_flag_process_check<false>(detail::enum_flag_processor_check{}, v, enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * constexpr e e1 = e::e;
	 * static_assert(check_any_enum_flag(e1));
	 * static_assert(not check_any_enum_flag(e1, e::b));
	 * static_assert(check_any_enum_flag(e1, e::b, e::c, e::d));
	 * static_assert(check_any_enum_flag(e1, e::b, e::c, e::d));
	 *
	 * @endcode
	 */
	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr bool check_any_enum_flag(Enum e, Enums ... enums) noexcept { return check_any_enum_flag(static_cast<std::underlying_type_t<Enum>>(e), enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * std::uint32_t flag = 0x0001 | 0x0010;
	 * flip_enum_flag_set(flag, e::b, e::c, e::d);
	 * assert(flag = 0x0001 | 0x0100 | 0x1000);
	 *
	 * @endcode
	 */
	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr void flip_enum_flag_set(Value& v, Enums ... enums) noexcept { enum_flag_process_set<false, false>(detail::enum_flag_processor_flip{}, v, enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * static_assert(flip_enum_flag_ret(0x0001 | 0x0010, e::b, e::c, e::d) == 0x0001 | 0x0100 | 0x1000);
	 *
	 * @endcode
	 */
	template<typename Value, typename... Enums>
		requires std::is_scalar_v<Value> &&
		         (std::is_enum_v<Enums> && ...) &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, Value> && ...)
	constexpr Value flip_enum_flag_ret(Value& v, Enums ... enums) noexcept { return enum_flag_process_ret<false, false>(detail::enum_flag_processor_flip{}, v, enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * e e1 = e;
	 * flip_enum_flag_set(e1, e::c, e::d);
	 * assert(e1 == e::a);
	 *
	 * @endcode
	 */
	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr void flip_enum_flag_set(Enum& e, Enums ... enums) noexcept { flip_enum_flag_set(static_cast<std::underlying_type_t<Enum>&>(e), enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * static_assert(flip_enum_flag_ret(e::e, e::c, e::d) == e::a);
	 *
	 * @endcode
	 */
	template<typename Enum, typename... Enums>
		requires std::is_enum_v<Enum> &&
		         (std::is_enum_v<Enums> && ...) &&
		         std::is_scalar_v<std::underlying_type_t<Enum>> &&
		         (std::is_convertible_v<std::underlying_type_t<Enums>, std::underlying_type_t<Enum>> && ...)
	constexpr Enum flip_enum_flag_ret(Enum e, Enums ... enums) noexcept { return static_cast<Enum>(flip_enum_flag_ret(static_cast<std::underlying_type_t<Enum>>(e), enums...)); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * constexpr e e1 = e::e;
	 * static_assert(is_all_enum_of(e1));
	 * static_assert(is_all_enum_of(e1, e::c, e::d));
	 * static_assert(not is_all_enum_of(e1, e::b, e::c, e::d));
	 *
	 * @endcode
	 */
	template<typename T, typename... Args>
		requires std::is_enum_v<T> && ((std::is_convertible_v<Args, T> || std::is_convertible_v<Args, std::underlying_type_t<T>>) && ...)
	constexpr bool is_all_enum_of(T current_enum, Args ... requires_enums) noexcept { return check_all_enum_flag(current_enum, requires_enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * constexpr e e1 = e::e;
	 * static_assert(is_any_enum_of(e1));
	 * static_assert(not is_any_enum_of(e1, e::b));
	 * static_assert(is_any_enum_of(e1, e::b, e::c, e::d));
	 * static_assert(is_any_enum_of(e1, e::b, e::c, e::d));
	 *
	 * @endcode
	 */
	template<typename T, typename... Args>
		requires std::is_enum_v<T> && ((std::is_convertible_v<Args, T> || std::is_convertible_v<Args, std::underlying_type_t<T>>) && ...)
	constexpr bool is_any_enum_of(T current_enum, Args ... requires_enums) noexcept { return check_any_enum_flag(current_enum, requires_enums...); }

	/**
	 * @code
	 *
	 * enum class e : std::uint32_t
	 * {
	 *	a = 0x0001,
	 *	b = 0x0010,
	 *	c = 0x0100,
	 *	d = 0x1000,
	 *	e = a | c | d,
	 * }
	 *
	 * constexpr e e1 = e::b;
	 * static_assert(is_enum_between_of(e1, e::a, e::c));
	 *
	 * @endcode
	 */
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
