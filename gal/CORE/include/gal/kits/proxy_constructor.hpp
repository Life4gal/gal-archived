#pragma once

#ifndef GAL_LANG_KITS_PROXY_CONSTRUCTOR_HPP
#define GAL_LANG_KITS_PROXY_CONSTRUCTOR_HPP

#include<gal/kits/proxy_function.hpp>

namespace gal::lang::kits
{
	namespace detail
	{
		template<typename Class, typename... Params>
		proxy_function do_make_constructor(Class (*)(Params ...))
		{
			if constexpr (not std::is_copy_constructible_v<Class>)
			{
				auto call = []<typename... Ps>(Ps&& ... params) { return std::make_shared<Class>(std::forward<Ps>(params)...); };

				return std::make_shared<proxy_function_callable<std::shared_ptr<Class>(Params ...), decltype(call)>>(
						std::move(call));
			}
			else
			{
				auto call = []<typename... Ps>(Ps&& ... params) { return Class{std::forward<Ps>(params)...}; };

				return std::make_shared<proxy_function_callable<Class(Params ...), decltype(call)>>(
						std::move(call));
			}
		}
	}

	template<typename Signature>
	proxy_function make_constructor() { return detail::do_make_constructor(static_cast<Signature*>(nullptr)); }
}

#endif // GAL_LANG_KITS_PROXY_CONSTRUCTOR_HPP
