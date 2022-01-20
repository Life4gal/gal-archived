#pragma once

#ifndef GAL_LANG_UTILS_FUNCTION_HPP
#define GAL_LANG_UTILS_FUNCTION_HPP

#include <type_traits>

namespace gal::utils
{
	template<typename FunctionType>
	struct y_combinator
	{
		using function_type = FunctionType;

		function_type function;

		template<typename... Args>
		constexpr decltype(auto) operator()(Args&&... args) const
		noexcept(std::is_nothrow_invocable_v<function_type, decltype(*this), Args...>)
		{
			// we pass ourselves to f, then the arguments.
			// the lambda should take the first argument as `auto&& recurse` or similar.
			return function(*this, std::forward<Args>(args)...);
		}
	};

	template<typename F>
	y_combinator(F) -> y_combinator<std::decay_t<F>>;
}

#endif // GAL_LANG_UTILS_FUNCTION_HPP
