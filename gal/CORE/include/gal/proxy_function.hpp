#pragma once

#ifndef GAL_LANG_PROXY_FUNCTION_HPP
	#define GAL_LANG_PROXY_FUNCTION_HPP

	#include <gal/foundation/proxy_function.hpp>
	#include <utils/algorithm.hpp>

namespace gal::lang
{
	template<typename Functions>
	requires std::is_same_v<typename Functions::value_type, foundation::immutable_proxy_function> || std::is_same_v<typename Functions::value_type, foundation::mutable_proxy_function>
	[[nodiscard]] foundation::boxed_value dispatch(
			const Functions&						 functions,
			const foundation::parameters_view_type	 parameters,
			const foundation::type_conversion_state& conversion)
	{
		std::vector<std::pair<std::size_t, std::reference_wrapper<const foundation::proxy_function_base>>> ordered_functions{};
		ordered_functions.reserve(functions.size());

		std::ranges::for_each(
				functions,
				[&ordered_functions, parameters](const auto& function)
				{
					if (const auto arity = function->get_arity();
						arity == foundation::proxy_function_base::no_parameters_arity) { ordered_functions.emplace_back(parameters.size(), std::cref(*function)); }
					else if (arity == static_cast<foundation::proxy_function_base::arity_size_type>(parameters.size()))
					{
						std::size_t num_diffs = 0;

						utils::zip_invoke(
								[&num_diffs](const auto& type, const auto& object)
								{ if (not type.bare_equal(object.type_info())) { ++num_diffs; } },
								function->types().begin() + 1,
								function->types().end(),
								parameters.begin());

						ordered_functions.emplace_back(num_diffs, std::cref(*function));
					}
				});

		for (decltype(parameters.size()) i = 0; i < parameters.size(); ++i)
		{
			for (const auto& [order, function]: ordered_functions)
			{
				using namespace foundation::exception;
				try
				{
					if (order == i && (i == 0 || function.get().filter(parameters, conversion))) { return function.get()(parameters, conversion); }
				}
				catch (const bad_boxed_cast&)
				{
					// parameter failed to cast, try again
				}
				catch (const arity_error&)
				{
					// invalid num params, try again
				}
				catch (const guard_error&)
				{
					// guard failed to allow the function to execute, try again
				}
			}
		}

		return foundation::proxy_function_detail::dispatch_with_conversion(
				ordered_functions |
						std::views::values |
						std::views::transform([](const std::reference_wrapper<const foundation::proxy_function_base>& f) -> const foundation::proxy_function_base&
											  { return f.get(); }),
				parameters,
				conversion,
				functions);
	}
}// namespace gal::lang

#endif// GAL_LANG_PROXY_FUNCTION_HPP
