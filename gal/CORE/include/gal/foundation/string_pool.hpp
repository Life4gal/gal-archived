#pragma once

#ifndef GAL_LANG_FOUNDATION_STRING_POOL_HPP
	#define GAL_LANG_FOUNDATION_STRING_POOL_HPP

#include <utils/string_pool.hpp>
#include <gal/foundation/string.hpp>

namespace gal::lang::foundation
{
	using string_pool_type = utils::string_pool<string_type::value_type, false, string_type::traits_type>;
}

#endif // GAL_LANG_FOUNDATION_STRING_POOL_HPP
