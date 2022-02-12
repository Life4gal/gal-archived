#pragma once

#ifndef GAL_UTILS_SOURCE_LOCATION_HPP
#define GAL_UTILS_SOURCE_LOCATION_HPP

#if __has_include(<source_location>)
	#include <source_location>
	using std_source_location = std::source_location;
#elif __has_include(<experimental/source_location>)
	#include <experimental/source_location>
	using std_source_location = std::experimental::source_location;
#else
	#error "The compiler does not support <source_location>"
	#endif

#endif//GAL_UTILS_SOURCE_LOCATION_HPP
