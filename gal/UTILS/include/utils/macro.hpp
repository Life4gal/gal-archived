#pragma once

#ifndef GAL_LANG_UTILS_MACRO_HPP
	#define GAL_LANG_UTILS_MACRO_HPP

namespace gal
{
	// Tell the compiler that this part of the code will never be reached.
	#if defined(_MSC_VER)
		#define UNREACHABLE() __assume(0)
	#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
		#define UNREACHABLE() __builtin_unreachable()
	#else
		#define UNREACHABLE()
	#endif
}

#endif // GAL_LANG_UTILS_MACRO_HPP
