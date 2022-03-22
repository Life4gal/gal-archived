#pragma once

#ifndef GAL_LANG_FUNCTOR_MAKER_HPP
#define GAL_LANG_FUNCTOR_MAKER_HPP

#include <gal/foundation/functor_maker.hpp>
#include <gal/foundation/type_info.hpp>

namespace gal::lang
{
	/**
	 * @brief Build a function caller that knows how to dispatch on a set of functions
	 *
	 * @param functions The set of functions to dispatch on.
	 * @param conversion
	 * @return A std::function object for dispatching
	 */
	template<typename FunctionSignature>
	std::function<FunctionSignature> make_functor(
			foundation::immutable_proxy_functions_type&& functions,
			const foundation::type_conversion_state* conversion)
	{
		const auto has_arity_match = std::ranges::any_of(
				functions,
				[](const auto& function)
				{
					return function->get_arity() == foundation::proxy_function_base::no_parameters_arity ||
					       foundation::functor_maker_detail::arity(static_cast<FunctionSignature*>(nullptr)) == static_cast<decltype(foundation::functor_maker_detail::arity(static_cast<FunctionSignature*>(nullptr)))>(function->get_arity());
				});

		if (not has_arity_match) { throw exception::bad_boxed_cast{foundation::make_type_info<foundation::immutable_proxy_function>(), typeid(std::function<FunctionSignature>)}; }
		return foundation::functor_maker_detail::make_function_invoker(static_cast<FunctionSignature*>(nullptr), std::move(functions), conversion);
	}

	/**
	 * @brief Build a function caller for a particular proxy_function object.
	 * useful in the case that a function is being pass out from scripting back
	 * into code
	 *
	 * @param function A function to execute.
	 * @param conversion
	 * @return A std::function object for dispatching
	 */
	template<typename FunctionSignature>
	std::function<FunctionSignature> make_functor(
			foundation::immutable_proxy_function&& function,
			const foundation::type_conversion_state* conversion) { return make_functor<FunctionSignature>(foundation::immutable_proxy_functions_type{std::move(function)}, conversion); }

	/**
	 * @brief Helper for automatically unboxing a boxed_value that contains
	 * a function object and creating a type safe C++ function caller from it.
	 */
	template<typename FunctionSignature>
	std::function<FunctionSignature> make_functor(
			const foundation::boxed_value& object,
			const foundation::type_conversion_state* conversion) { return make_functor<FunctionSignature>(boxed_cast<foundation::immutable_proxy_function>(object, conversion), conversion); }
}// namespace gal::lang

#endif//GAL_LANG_FUNCTOR_MAKER_HPP
