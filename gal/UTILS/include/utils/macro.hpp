#pragma once

#ifndef GAL_UTILS_MACRO_HPP
#define GAL_UTILS_MACRO_HPP

#ifdef _MSC_VER
	#define UNREACHABLE() __assume(0)
	#define DEBUG_TRAP() __debugbreak()
	#define IMPORTED_SYMBOL __declspec(dllimport)
	#define EXPORTED_SYMBOL __declspec(dllexport)
	#define LOCAL_SYMBOL
#else
	#define UNREACHABLE() __builtin_unreachable()
	#define DEBUG_TRAP() __builtin_trap()
	#define IMPORTED_SYMBOL __attribute__((visibility("default")))
	#define EXPORTED_SYMBOL __attribute__((visibility("default")))
	#define LOCAL_SYMBOL __attribute__((visibility("hidden")))
#endif// _MSC_VER

#endif // GAL_UTILS_MACRO_HPP
