#pragma once

#ifndef GAL_LANG_UTILS_MACRO_HPP
	#define GAL_LANG_UTILS_MACRO_HPP

namespace gal
{
	#ifdef _MSC_VER
		#define UNREACHABLE() __assume(0)
		#define DEBUG_TRAP() __debugbreak()
	#else
		#define UNREACHABLE() __builtin_unreachable()
		#define DEBUG_TRAP() __builtin_trap()
	#endif// _MSC_VER
}

#endif // GAL_LANG_UTILS_MACRO_HPP
