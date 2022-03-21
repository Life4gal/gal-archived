#pragma once

#ifndef GAL_LANG_FOUNDATION_FUNCTION_REGISTER_HPP
	#define GAL_LANG_FOUNDATION_FUNCTION_REGISTER_HPP

	#include <gal/foundation/proxy_function.hpp>
	#include <utils/function_signature.hpp>

namespace gal::lang::foundation
{
	class function_register
	{
		template<typename Function, typename ReturnType, typename... Params, bool IsNoexcept, bool IsMember, bool IsMemberObject, bool IsObject>
		[[nodiscard]] static proxy_function do_register_function(Function&& function, utils::function_signature_t<ReturnType, IsNoexcept, IsMember, IsMemberObject, IsObject, Params...> signature)
		{
			if constexpr (IsMemberObject)
			{
				// we now that the Param pack will have only one element, so we are safe expanding it here
				return std::make_shared<member_accessor<ReturnType, std::decay_t<Params>...>>(std::forward<Function>(function));
			}
			else if constexpr (IsMember)
			{
				auto call = [function = std::forward<Function>(function), signature]<typename... Ps>(auto&& self, Ps&&... params) noexcept(IsNoexcept)->decltype(auto)
				{
					return (utils::get_object_instance(signature, self).*function)(std::forward<Ps>(params)...);
				};
				return std::make_shared<proxy_function_callable<ReturnType(Params...), decltype(call)>>(std::move(call));
			}
			else
			{
				return std::make_shared<proxy_function_callable<ReturnType(Params...), std::decay_t<Function>>>(std::forward<Function>(function));
			}
		}

		template<typename Class, typename... Params>
		[[nodiscard]] static proxy_function do_register_constructor(Class (*)(Params...))
		{
			if constexpr (not std::is_copy_assignable_v<Class>)
			{
				auto call = []<typename... Ps>(Ps && ... params)
				{
					return std::make_shared<Class>(std::forward<Ps>(params)...);
				};
				return std::make_shared<proxy_function_callable<std::shared_ptr<Class>(Params...), decltype(call)>>(std::move(call));
			}
			else
			{
				auto call = []<typename... Ps>(Ps && ... params)
				{
					return Class{std::forward<Ps>(params)...};
				};
				return std::make_shared<proxy_function_callable<Class(Params...), decltype(call)>>(std::move(call));
			}
		}

	public:
		/**
		 * @brief this version peels off the function object itself from the function signature
		 * when used on a callable object.
		 */
		template<typename Function, typename ReturnType, typename Object, typename... Param, bool IsNoexcept>
		[[nodiscard]] static proxy_function register_function(Function&& function, utils::function_signature_t<ReturnType, IsNoexcept, false, false, true, Object, Param...>)
		{
			return do_register_function(std::forward<Function>(function), utils::function_signature_t<ReturnType, IsNoexcept, false, false, true, Param...>{});
		}

		template<typename Function, typename ReturnType, typename... Param, bool IsNoexcept, bool IsMember, bool IsMemberObject>
		[[nodiscard]] static proxy_function register_function(Function&& function, utils::function_signature_t<ReturnType, IsNoexcept, IsMember, IsMemberObject, false, Param...> signature)
		{
			return do_register_function(std::forward<Function>(function), signature);
		}

		template<typename ConstructorSignature>
		[[nodiscard]] static proxy_function register_constructor()
		{
			return do_register_constructor(static_cast<ConstructorSignature*>(nullptr));
		}
	};
}// namespace gal::lang::foundation

#endif//GAL_LANG_FOUNDATION_FUNCTION_REGISTER_HPP
