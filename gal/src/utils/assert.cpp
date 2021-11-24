#include <utils/assert.hpp>

#ifdef GAL_FMT_NOT_SUPPORT
	#include <fmt/format.h>
#else
	#include <format>
#endif

#include <iostream>

namespace gal
{
	void gal_assert(
			[[maybe_unused]] bool						condition,
			[[maybe_unused]] std::string_view			message,
			[[maybe_unused]] const std_source_location& location) noexcept
	{
#ifndef NDEBUG
		if (not condition)
		{
			// todo: output to other places, or you can specify the output location

	#ifdef GAL_FMT_NOT_SUPPORT
			std::cerr << fmt::format("[FILE: {} -> FUNCTION: {} -> LINE: {}] assert failed: {}\n", location.file_name(), location.function_name(), location.line(), message);
	#else
			std::cerr << std::format("[FILE: {} -> FUNCTION: {} -> LINE: {}] assert failed: {}\n", location.file_name(), location.function_name(), location.line(), message);
	#endif
			std::exit(-1);
		}
#endif
	}
}// namespace gal
