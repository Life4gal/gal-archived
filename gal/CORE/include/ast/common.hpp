#pragma once

#ifndef GAL_LANG_AST_COMMON_HPP
	#define GAL_LANG_AST_COMMON_HPP

#include <string_view>

namespace gal::ast
{
	using gal_null_type = std::nullptr_t;
	using gal_boolean_type = bool;
	using gal_number_type = double;
	using gal_string_type = std::string_view;
}

#endif // GAL_LANG_AST_COMMON_HPP
