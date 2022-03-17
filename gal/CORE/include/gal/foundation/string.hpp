#pragma once

#ifndef GAL_LANG_STRING_HPP
	#define GAL_LANG_STRING_HPP

#include<string>
#include<string_view>

namespace gal::lang::foundation
{
	// todo: all types that store strings only store string_view, and the use of string memory is handed over to string_pool

	using string_type = std::string;
	using string_view_type = std::string_view;
}

#endif // GAL_LANG_STRING_HPP
