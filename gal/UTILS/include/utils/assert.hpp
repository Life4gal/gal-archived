#pragma once

#ifndef GAL_UTILS_ASSERT_HPP
#define GAL_UTILS_ASSERT_HPP

#ifndef GAL_NO_ASSERT
#ifdef NDEBUG
			#define GAL_NO_ASSERT
#endif
#endif

#ifndef GAL_NO_ASSERT
#if __has_include(<source_location>)
#include <source_location>
using std_source_location = std::source_location;
#elif __has_include(<experimental/source_location>)
		#include <experimental/source_location>
using std_source_location = std::experimental::source_location;
#else
		#error "assert requires <source_location>"
#endif

#include <string_view>
#endif

namespace gal
{
	#ifndef GAL_NO_ASSERT
	#define GAL_ASSERT_CONSTEXPR inline
	void gal_assert(
			bool condition,
			std::string_view message = {"no details"},
			const std_source_location& location = std_source_location::current()) noexcept;
	#else
		#define gal_assert(...)
		#define GAL_ASSERT_CONSTEXPR constexpr
	#endif
}// namespace gal

#endif//GAL_UTILS_ASSERT_HPP
