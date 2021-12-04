#pragma once

#ifndef GAL_LANG_UTILS_ASSERT_HPP
	#define GAL_LANG_UTILS_ASSERT_HPP

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

namespace gal
{
	void gal_assert(
			[[maybe_unused]] bool						condition,
			[[maybe_unused]] std::string_view			message	 = {"no details"},
			[[maybe_unused]] const std_source_location& location = std_source_location::current()) noexcept;
}// namespace gal


#endif//GAL_LANG_ASSERT_HPP
