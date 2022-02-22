#pragma once

#ifndef GAL_LANG_KITS_CALL_FUNCTION_HPP
#define GAL_LANG_KITS_CALL_FUNCTION_HPP

#include <gal/kits/boxed_value_cast.hpp>
#include <gal/kits/proxy_function.hpp>

namespace gal::lang::kits
{
	namespace detail
	{
		using function_invoker_functions_type = std::vector<const_proxy_function>;

		/**
		 * @brief used internally for unwrapping a function call's types
		 */
		template<typename Result, typename...>
		struct function_invoker
		{
			function_invoker_functions_type functions;
			const type_conversion_manager* manager;

			function_invoker(
					function_invoker_functions_type functions,
					const type_conversion_manager* manager)
				: functions{std::move(functions)},
				  manager{manager} {}

			template<typename P, typename Q>
			static boxed_value box(Q&& q)
			{
				if constexpr (std::is_same_v<std::decay_t<Q>, boxed_value>) { return std::forward<Q>(q); }
				else if constexpr (std::is_reference_v<P>) { return {std::ref(std::forward<Q>(q))}; }
				else { return {std::forward<Q>(q)}; }
			}

			Result invoke(
					const function_parameters& parameters,
					const type_conversion_state& conversion
					)
			{
				if constexpr (std::is_arithmetic_v<Result> && not std::is_same_v<std::remove_cvref_t<Result>, bool>) { return boxed_number{dispatch(functions, parameters, conversion)}.as<Result>(); }
				else if constexpr (std::is_same_v<Result, void>) { return dispatch(functions, parameters, conversion); }
				else { return boxed_cast<Result>(dispatch(functions, parameters, conversion), &conversion); }
			}

			template<typename... P>
			Result operator()(P&&... param)
			{
				std::array<boxed_value, sizeof...(P)> params{box<P>(std::forward<P>(param))...};

				if (manager)
				{
					return invoke(
							function_parameters{params},
							{*manager, manager->get_conversion_saves()});
				}

				type_conversion_manager dummy;
				return invoke(
						function_parameters{params},
						{dummy, dummy.get_conversion_saves()});
			}
		};

		template<typename Result, typename... Params>
		std::function<Result(Params ...)> make_function_invoker(
				Result (Params ...),
				const function_invoker_functions_type& functions,
				const type_conversion_state* conversion
				) { return std::function<Result(Params ...)>(function_invoker<Result, Params...>(functions, conversion ? conversion->operator->() : nullptr)); }

		template<typename Result, typename... Param>
		constexpr auto arity(Result (*)(Param ...)) noexcept { return sizeof...(Param); }
	}

	/**
	 * @brief Build a function caller that knows how to dispatch on a set of functions
	 *
	 * @param functions The set of functions to dispatch on.
	 * @param conversion
	 * @return A std::function object for dispatching
	 */
	template<typename Function>
	std::function<Function> make_functor(
			const std::vector<const_proxy_function>& functions,
			const type_conversion_state* conversion)
	{
		const auto has_arity_match = std::ranges::any_of(
				functions,
				[](const auto& function)
				{
					return function->get_arity() == proxy_function_base::no_parameters_arity ||
					       static_cast<decltype(detail::arity(static_cast<Function*>(nullptr)))>(function->get_arity()) == detail::arity(static_cast<Function*>(nullptr));
				});

		if (not has_arity_match) { throw bad_boxed_cast{utility::make_type_info<const_proxy_function>(), typeid(std::function<Function>)}; }

		return detail::make_function_invoker(static_cast<Function*>(nullptr), functions, conversion);
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
	template<typename Function>
	std::function<Function> make_functor(
			const_proxy_function function,
			const type_conversion_state* conversion
			)
	{
		return make_functor<Function>(
				std::vector{std::move(function)},
				conversion);
	}

	/**
	 * @brief Helper for automatically unboxing a boxed_value that contains
	 * a function object and creating a type safe C++ function caller from it.
	 */
	template<typename Function>
	std::function<Function> make_functor(
			const boxed_value& object,
			const type_conversion_state* conversion
			) { return make_functor<Function>(boxed_cast<const_proxy_function>(object, conversion), conversion); }

	namespace detail
	{
		/**
		 * @brief cast invoker to handle automatic casting to std::function
		 */
		template<typename FunctionSignature>
		struct cast_invoker<std::function<FunctionSignature>>
		{
			static std::function<FunctionSignature> cast(
					const boxed_value& object,
					const type_conversion_state* conversion
					)
			{
				if (object.type_info().bare_equal(utility::make_type_info<const_proxy_function>())) { return make_functor<FunctionSignature>(object, conversion); }
				return default_cast_invoker<std::function<FunctionSignature>>::cast(object, conversion);
			}
		};

		/**
		 * @brief cast invoker to handle automatic casting to const std::function
		 */
		template<typename FunctionSignature>
		struct cast_invoker<const std::function<FunctionSignature>>
		{
			static std::function<FunctionSignature> cast(
					const boxed_value& object,
					const type_conversion_state* conversion)
			{
				if (object.type_info().bare_equal(utility::make_type_info<const_proxy_function>())) { return make_functor<FunctionSignature>(object, conversion); }
				return default_cast_invoker<const std::function<FunctionSignature>>::cast(object, conversion);
			}
		};

		/**
		 * @brief cast invoker to handle automatic casting to const std::function&
		 */
		template<typename FunctionSignature>
		struct cast_invoker<const std::function<FunctionSignature>&>
		{
			static std::function<FunctionSignature> cast(
					const boxed_value& object,
					const type_conversion_state* conversion
					)
			{
				if (object.type_info().bare_equal(utility::make_type_info<const_proxy_function>())) { return make_functor<FunctionSignature>(object, conversion); }
				return default_cast_invoker<const std::function<FunctionSignature>&>::cast(object, conversion);
			}
		};
	}
}

#endif // GAL_LANG_KITS_CALL_FUNCTION_HPP
