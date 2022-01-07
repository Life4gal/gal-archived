#pragma once

#ifndef GAL_LANG_COMPILE_HPP
#define GAL_LANG_COMPILE_HPP

#include<compile_options.hpp>
#include<string_view>

namespace gal
{
	/**
	 * @brief compile source to bytecode; when source compilation fails, the resulting bytecode contains the encoded error.
	 */
	std::string compile(std::string_view source, compile_options option);
}

#endif // AL_LANG_COMPILE_HPP
