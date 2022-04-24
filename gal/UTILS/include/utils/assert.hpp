#pragma once

#ifndef GAL_UTILS_ASSERT_HPP
#define GAL_UTILS_ASSERT_HPP

#ifndef GAL_NO_ASSERT
#ifdef NDEBUG
			#define GAL_NO_ASSERT
#endif
#endif

#ifndef GAL_NO_ASSERT
#include <utils/source_location.hpp>
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
		#define GAL_ASSERT_CONSTEXPR constexpr
		#define gal_assert(...) 
	#endif
}// namespace gal

#endif//GAL_UTILS_ASSERT_HPP
