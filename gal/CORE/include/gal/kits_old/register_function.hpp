#pragma once

#ifndef GAL_LANG_KITS_REGISTER_FUNCTION_HPP
#define GAL_LANG_KITS_REGISTER_FUNCTION_HPP

#include<utils/function_signature.hpp>
#include<gal/kits/proxy_function.hpp>

namespace gal::lang::kits
{
	namespace detail
	{
		template<typename Function, typename ReturnType, typename... Params, bool IsNoexcept, bool IsMember, bool IsMemberObject, bool IsObject>
		proxy_function do_make_callable_impl(Function&& function, utils::function_signature_t<ReturnType, IsNoexcept, IsMember, IsMemberObject, IsObject, Params...>)
		{
			if constexpr (IsMemberObject)
			{
				// we now that the Param pack will have only one element, so we are safe expanding it here
				return std::make_shared<attribute_accessor<ReturnType, std::decay_t<Params>...>>(
						std::forward<Function>(function));
			}
			else if constexpr (IsMember)
			{
				auto call = [function = std::forward<Function>(function)]<typename... Ps>(auto&& object, Ps&&... param) noexcept(IsNoexcept) -> decltype(auto) { return (utils::get_object_instance(utils::function_signature_t<ReturnType, IsNoexcept, IsMember, IsMemberObject, IsObject, Params...>{}, object).*function)(std::forward<Ps>(param)...); };
				return std::make_shared<proxy_function_callable<ReturnType(Params ...), decltype(call)>>(
						std::move(call));
			}
			else
			{
				return std::make_shared<proxy_function_callable<ReturnType(Params ...), std::decay_t<Function>>>(
						std::forward<Function>(function));
			}
		}

		/**
		 * @brief this version peels off the function object itself from the function signature
		 * when used on a callable object.
		 */
		template<typename Function, typename ReturnType, typename Object, typename... Param, bool IsNoexcept>
		proxy_function do_make_callable(Function&& function, utils::function_signature_t<ReturnType, IsNoexcept, false, false, true, Object, Param...>) { return do_make_callable_impl(std::forward<Function>(function), utils::function_signature_t<ReturnType, IsNoexcept, false, false, true, Param...>{}); }

		template<typename Function, typename ReturnType, typename... Param, bool IsNoexcept, bool IsMember, bool IsMemberObject>
		proxy_function do_make_callable(Function&& function, utils::function_signature_t<ReturnType, IsNoexcept, IsMember, IsMemberObject, false, Param...> signature) { return do_make_callable_impl(std::forward<Function>(function), signature); }
	}

	/**
	 * @brief Creates a new proxy_function object from a free function, member function or data member.
	 *
	 * @param function Function / member to expose.
	 *
	 * @code
	 * fun(&a_free_function)
	 * fun(&a_class::a_member_function)
	 * fun(&a_class::a_member_data)
	 * @endcode 
	 */
	template<typename Function>
	[[nodiscard]] proxy_function fun(Function&& function) { return detail::do_make_callable(std::forward<Function>(function), utils::make_function_signature(function)); }

	/**
	 * @brief Creates a new proxy_function object from a free function, member function or data member.
	 * @note It is allowed to bind the first n parameters of the function first, if it is not the first n parameters then a lambda should be used.
	 *
	 * @code
	 * a_class a;
	 * fun(&a_class::a_member_function, std::ref(a))
	 * @endcode 
	 */
	template<typename Function, typename... Param>
	[[nodiscard]] proxy_function fun(Function&& function, Param&&... param) { return fun(std::bind_front(std::forward<Function>(function), std::forward<Param>(param)...)); }
}

#endif // GAL_LANG_KITS_REGISTER_FUNCTION_HPP
