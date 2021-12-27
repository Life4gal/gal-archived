#pragma once

#ifndef GAL_LANG_UTILS_CONCEPT_HPP
#define GAL_LANG_UTILS_CONCEPT_HPP

#include <type_traits>

namespace gal::utils
{
	template<typename T, template<typename, typename> typename Requirement, typename... Ts>
	struct is_any_requires_of : std::disjunction<Requirement<T, Ts>...> { };

	template<typename T, template<typename, typename> typename Requirement, typename... Ts>
	constexpr static bool is_any_requires_of_v = is_any_requires_of<T, Requirement, Ts...>::value;
	template<typename T, template<typename, typename> typename Requirement, typename... Ts>
	concept any_requires_of_t = is_any_requires_of_v<T, Requirement, Ts...>;

	template<typename T, template<typename, typename> typename Requirement, typename... Ts>
	struct is_all_requires_of : std::conjunction<Requirement<T, Ts>...> { };

	template<typename T, template<typename, typename> typename Requirement, typename... Ts>
	constexpr static bool is_all_requires_of_v = is_all_requires_of<T, Requirement, Ts...>::value;
	template<typename T, template<typename, typename> typename Requirement, typename... Ts>
	concept all_requires_of_t = is_all_requires_of_v<T, Requirement, Ts...>;

	template<typename T, typename... Ts>
	struct is_any_type_of : is_any_requires_of<T, std::is_same, Ts...> { };

	template<typename T, typename... Ts>
	constexpr static bool is_any_type_of_v = is_any_type_of<T, Ts...>::value;
	template<typename T, typename... Ts>
	concept any_type_of_t = is_any_type_of_v<T, Ts...>;

	template<typename T, typename... Ts>
	struct is_all_type_of : is_all_requires_of<T, std::is_same, Ts...> { };

	template<typename T, typename... Ts>
	constexpr static bool is_all_type_of_v = is_all_type_of<T, Ts...>::value;
	template<typename T, typename... Ts>
	concept all_type_of_t = is_all_type_of_v<T, Ts...>;
}

#endif// GAL_LANG_UTILS_CONCEPT_HPP
