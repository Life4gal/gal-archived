#pragma once

#ifndef GAL_UTILS_MACRO_HPP
#define GAL_UTILS_MACRO_HPP

#ifdef _MSC_VER
	#define COMPILER_MSVC
	#define UNREACHABLE() __assume(0)
	#define DEBUG_TRAP() __debugbreak()
	#define IMPORTED_SYMBOL __declspec(dllimport)
	#define EXPORTED_SYMBOL __declspec(dllexport)
	#define LOCAL_SYMBOL

	#define DISABLE_WARNING_PUSH __pragma(warning(push))
	#define DISABLE_WARNING_POP __pragma(warning(pop))
	#define DISABLE_WARNING(warningNumber) __pragma(warning(disable : \
		warningNumber))

#else
	#define COMPILER_GCC
	#define UNREACHABLE() __builtin_unreachable()
	#define DEBUG_TRAP() __builtin_trap()
	#define IMPORTED_SYMBOL __attribute__((visibility("default")))
	#define EXPORTED_SYMBOL __attribute__((visibility("default")))
	#define LOCAL_SYMBOL __attribute__((visibility("hidden")))

	#define DISABLE_WARNING_PUSH _Pragma(GCC diagnostic push)
	#define DISABLE_WARNING_POP _Pragma(GCC diagnostic pop)
	#define DISABLE_WARNING(warningName) _Pragma(GCC diagnostic ignored #warningName)
#endif// _MSC_VER

#endif // GAL_UTILS_MACRO_HPP
