#pragma once

#ifndef GAL_LANG_COMPILE_HPP
#define GAL_LANG_COMPILE_HPP

#ifdef __cplusplus
extern "C"
{
#endif

#include<stddef.h>

typedef struct gal_compile_options_tag
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

	// global builtin to construct vectors; disabled by default
	const char*	 vector_lib;
	const char*	 vector_ctor;

	// null-terminated array of globals that are mutable; disables the import optimization for fields accessed through these
	const char** mutable_globals;
} gal_compile_options;

/* compile source to bytecode; when source compilation fails, the resulting bytecode contains the encoded error. use free() to destroy */
// todo: export ?
char* gal_compile(const char* source, size_t size, gal_compile_options* options, size_t* result_size);

#ifdef __cplusplus
}
#endif

#endif // GAL_LANG_COMPILE_HPP
