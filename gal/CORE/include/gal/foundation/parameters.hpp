#pragma once

#ifndef GAL_LANG_FOUNDATION_PARAMETERS_HPP
#define GAL_LANG_FOUNDATION_PARAMETERS_HPP

#include <gal/foundation/boxed_value.hpp>
#include <gal/foundation/type_info.hpp>
#include <gal/foundation/string.hpp>
#include <utils/container_view.hpp>
#include <vector>

namespace gal::lang::foundation
{
	using parameters_type = std::vector<boxed_value>;
	using parameters_view_type = utils::container_view<boxed_value>;

	using type_infos_type = std::vector<gal_type_info>;
	using type_infos_view_type = utils::container_view<gal_type_info>;

	using names_type = std::vector<string_type>;
	using names_view_type = utils::container_view<string_type>;

	using name_views_type = std::vector<string_view_type>;
	using name_views_view_type = utils::container_view<string_view_type>;

	class function_proxy_base;
	/**
	 * @brief Const version of proxy_function. Points to a const proxy_function.
	 * This is how most registered functions are handled internally.
	 */
	using const_function_proxy_type = std::shared_ptr<const function_proxy_base>;
	using const_function_proxies_type = std::vector<const_function_proxy_type>;
	using const_function_proxies_view_type = utils::container_view<const_function_proxy_type>;

	/**
	 * @brief Common typedef used for passing of any registered function in GAL.
	 */
	using mutable_function_proxy_type = std::shared_ptr<function_proxy_base>;
	using mutable_function_proxies_type = std::vector<mutable_function_proxy_type>;
	using mutable_function_proxies_view_type = utils::container_view<mutable_function_proxy_type>;

	using function_proxy_type = mutable_function_proxy_type;
	using function_proxies_type = mutable_function_proxies_type;
	using function_proxies_view_type = mutable_function_proxies_view_type;
}

#endif // GAL_LANG_FOUNDATION_PARAMETERS_HPP
