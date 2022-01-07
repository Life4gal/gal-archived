#pragma once

#ifndef GAL_LANG_COMPILE_OPTIONS_HPP
	#define GAL_LANG_COMPILE_OPTIONS_HPP

namespace gal
{
	struct compile_options
	{
		// 0 - no optimization
		// 1 - baseline optimization level that doesn't prevent debug-ability
		// 2 - includes optimizations that harm debug-ability such as inlining
		int			 optimization_level;// default=1

		// 0 - no debugging support
		// 1 - line info & function names only; sufficient for back traces
		// 2 - full debug info with local & upvalue names; necessary for debugger
		int			 debug_level;// default=1

		// 0 - no code coverage support
		// 1 - statement coverage
		// 2 - statement and expression coverage (verbose)
		int			 coverage_level;// default=0

		// null-terminated array of globals that are mutable; disables the import optimization for fields accessed through these
		const char** mutable_globals;
	};
}

#endif
