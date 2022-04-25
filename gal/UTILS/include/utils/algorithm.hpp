#pragma once

#ifndef GAL_UTILS_ALGORITHM_HPP
#define GAL_UTILS_ALGORITHM_HPP

#include <ranges>

namespace gal::utils
{
	template<typename Function, typename Iterator, typename... Iterators>
	constexpr void zip_invoke(
			Function&& function,
			Iterator begin,
			Iterator end,
			Iterators ... iterators
			)
	{
		auto real_function = [&function](auto&... is)
		{
			std::invoke(function, (*is)...);
			((++is), ...);
		};

		while (begin != end) { real_function(begin, iterators...); }
	}

	template<typename Function, typename... Iterator>
	constexpr void zip_invoke(
			Function&& function,
			std::ranges::range auto&& r,
			Iterator ... iterator
			)
	{
		utils::zip_invoke(
				std::forward<Function>(function),
				std::ranges::begin(r),
				std::ranges::end(r),
				iterator...);
	}
}

#endif // GAL_UTILS_ALGORITHM_HPP
