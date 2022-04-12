#pragma once

#ifndef GAL_UTILS_TYPE_TRAITS_HPP
#define GAL_UTILS_TYPE_TRAITS_HPP

namespace gal::utils
{
	namespace type_traits_detail
	{
		template<typename Default, typename Placeholder, template<typename...> typename Operand, typename... Args>
		struct detector : std::false_type
		{
			using type = Default;
		};

		template<typename Default, template<typename...> typename Operand, typename... Args>
		struct detector<Default, std::void_t<Operand<Args...>>, Operand, Args...> : std::true_type
		{
			using type = Operand<Args...>;
		};
	}// namespace detail

	struct detect_nonesuch_t {};

	template<template<typename...> typename Operand, typename... Args>
	constexpr bool is_detected_v = type_traits_detail::detector<detect_nonesuch_t, void, Operand, Args...>::value;

	template<template<typename...> typename Operand, typename... Args>
	using detected_t = typename type_traits_detail::detector<detect_nonesuch_t, void, Operand, Args...>::type;

	template<typename OrType, template<typename...> typename Operand, typename... Args>
	using detected_or_t = typename type_traits_detail::detector<OrType, void, Operand, Args...>::type;

	template<typename Expect, template<typename...> typename Operand, typename... Args>
	constexpr bool is_expect_detected_v = std::is_same_v<Expect, detected_t<Operand, Args...>>;

	template<typename To, template<typename...> typename Operand, typename... Args>
	constexpr bool is_convertible_detected_v = std::is_convertible_v<detected_t<Operand, Args...>, To>;

	template<typename Expect, typename OrType, template<typename...> typename Operand, typename... Args>
	constexpr bool is_expect_operator_or_v = std::is_same_v<Expect, detected_or_t<OrType, Operand, Args...>>;

	template<typename To, typename OrType, template<typename...> typename Operand, typename... Args>
	constexpr bool is_convertible_detected_or_v = std::is_convertible_v<detected_or_t<OrType, Operand, Args...>, To>;
}

#endif // GAL_UTILS_TYPE_TRAITS_HPP
