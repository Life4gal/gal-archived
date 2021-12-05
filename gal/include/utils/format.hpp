#ifndef GAL_LANG_UTILS_FORMAT_HPP
#define GAL_LANG_UTILS_FORMAT_HPP

#ifdef GAL_FMT_NOT_SUPPORT
	#include <fmt/format.h>
	namespace std_format = fmt;
#else
	#include <format>
	namespace std_format = std;
#endif

#endif//GAL_LANG_UTILS_FORMAT_HPP
