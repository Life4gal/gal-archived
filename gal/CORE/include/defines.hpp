#pragma once

#ifndef GAL_LANG_DEFINES_HPP
	#define GAL_LANG_DEFINES_HPP

#include<vector>
#include<string_view>
#include<string>
#include<memory>

#include<utils/fixed_string.hpp>

#define GAL_LANG_ARITHMETIC_DIVIDE_ZERO_PROTECT

namespace gal::lang
{
	// todo: It looks like now only GCC correctly recognizes the function signature
	// cannot use fixed_string::match with std::string
	using dynamic_object_name = GAL_UTILS_FIXED_STRING_TYPE("dynamic_object");
}

#endif // GAL_LANG_DEFINES_HPP
