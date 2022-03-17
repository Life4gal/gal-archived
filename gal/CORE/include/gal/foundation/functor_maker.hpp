#pragma once

#ifndef GAL_LANG_FOUNDATION_FUNCTOR_MAKER_HPP
	#define GAL_LANG_FOUNDATION_FUNCTOR_MAKER_HPP

	#include <gal/boxed_cast.hpp>
	#include <gal/foundation/dynamic_object.hpp>

namespace gal::lang::foundation
{
	namespace functor_maker_detail
	{
		/**
		 * @brief used internally for unwrapping a function call's types
		 */
		template<typename Result, typename...>
		struct function_invoker
		{
			immutable_proxy_functions_type functions;
			const type_conversion_manager* manager;

			template<typename P, typename Q>
			static boxed_value box(Q&& q)
			{
				if constexpr (std::is_same_v<std::decay_t<Q>, boxed_value>) { return std::forward<Q>(q); }
				else if constexpr (std::is_reference_v<P>)
				{
					return {std::ref(std::forward<Q>(q))};
				}
				else
				{
					return {std::forward<Q>(q)};
				}
			}

			Result do_invoke(const parameters_view_type params, const type_conversion_state& conversion)
			{
				if constexpr (std::is_arithmetic_v<Result> && not std::is_same_v<std::remove_cvref_t<Result>, bool>) { return boxed_number{dispatch(functions, params, conversion)}.as<Result>(); }
				else if constexpr (std::is_same_v<Result, void>)
				{
					return dispatch(functions, params, conversion);
				}
				else
				{
					return boxed_cast<Result>(dispatch(functions, params, conversion), &conversion);
				}
			}

			template<typename... P>
			Result operator()(P&&... params)
			{
				std::array<boxed_value, sizeof...(P)> ps{box<P>(std::forward<P>(params))...};

				if (manager)
				{
					return do_invoke(parameters_view_type{ps}, {*manager, manager->get_conversion_saves()});
				}

				type_conversion_manager dummy;
				return do_invoke(
						parameters_view_type{ps},
						{dummy, dummy.get_conversion_saves()});
			}
		};

		template<typename Result, typename... Params>
		[[nodiscard]] std::function<Result(Params...)> make_function_invoker(
				Result(Params...),
				immutable_proxy_functions_type&& functions,
				const type_conversion_state*	 conversion)
		{
			return std::function<Result(Params...)>(function_invoker<Result, Params...>{.functions = std::move(functions), .manager = conversion ? conversion->operator->() : nullptr});
		}

		template<typename Result, typename... Params>
		consteval auto arity(Result (*)(Params...)) noexcept
		{
			return sizeof...(Params);
		}
	}// namespace functor_maker_detail

	namespace boxed_cast_detail
	{
		/**
		 * @brief cast invoker to handle automatic casting to std::function
		 */
		template<typename FunctionSignature>
		struct cast_invoker<std::function<FunctionSignature>>
		{
			static std::function<FunctionSignature> cast(
					const boxed_value&			 object,
					const type_conversion_state* conversion)
			{
				if (object.type_info().bare_equal(make_type_info<immutable_proxy_function>()))
				{
					return make_functor<FunctionSignature>(object, conversion);
				}
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
					const boxed_value&			 object,
					const type_conversion_state* conversion)
			{
				if (object.type_info().bare_equal(make_type_info<immutable_proxy_function>()))
				{
					return make_functor<FunctionSignature>(object, conversion);
				}
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
					const boxed_value&			 object,
					const type_conversion_state* conversion)
			{
				if (object.type_info().bare_equal(make_type_info<immutable_proxy_function>()))
				{
					return make_functor<FunctionSignature>(object, conversion);
				}
				return default_cast_invoker<const std::function<FunctionSignature>&>::cast(object, conversion);
			}
		};
	}// namespace boxed_cast_detail
}// namespace gal::lang::foundation

#endif//GAL_LANG_FOUNDATION_FUNCTOR_MAKER_HPP
