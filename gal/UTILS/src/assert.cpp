#include <utils/assert.hpp>
#include <utils/macro.hpp>

#ifdef GAL_FMT_NOT_SUPPORT
#include <fmt/format.h>
#else
	#include <format>
#endif

#include <iostream>

namespace gal
{
	#ifndef GAL_NO_ASSERT
	void gal_assert(
			const bool condition,
			std::string_view message,
			const std_source_location& location) noexcept
	{
		if (not condition)
		{
			// todo: output to other places, or you can specify the output location

			#ifdef GAL_FMT_NOT_SUPPORT
			std::cerr << fmt::format("[FILE: {} -> FUNCTION: {} -> LINE: {}] assert failed: {}\n", location.file_name(), location.function_name(), location.line(), message);
			#else
			std::cerr << std::format("[FILE: {} -> FUNCTION: {} -> LINE: {}] assert failed: {}\n", location.file_name(), location.function_name(), location.line(), message);
			#endif
			DEBUG_TRAP();
			std::abort();
		}
	}
	#endif
}// namespace gal
