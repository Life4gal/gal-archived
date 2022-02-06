#pragma once

#ifndef GAL_LANG_DEF_HPP
#define GAL_LANG_DEF_HPP

#define GAL_LANG_DEBUG
#define GAL_LANG_REF_TRACE

#if defined(GAL_LANG_DEBUG)
#define GAL_LANG_DO_IF_DEBUG(...) __VA_ARGS__
#else
		#define #define GAL_LANG_DO_IF_DEBUG(...)
#endif

#include <utility>
#include <utils/macro.hpp>

namespace gal::lang
{
	using gal_size_type = std::size_t;

	constexpr gal_size_type invalid_size = static_cast<gal_size_type>(-1);

	using gal_hash_type = std::size_t;

	#if !defined(GAL_API_CLASS)
	#define GAL_API_CLASS EXPORTED_SYMBOL
	#endif

	#if !defined(GAL_API_FUNC)
	#define GAL_API_FUNC EXPORTED_SYMBOL
	#endif

	#if !defined(GAL_API_DATA)
	#define GAL_API_DATA extern EXPORTED_SYMBOL
	#endif
}

#endif // GAL_LANG_DEF_HPP
