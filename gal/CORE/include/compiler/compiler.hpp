#pragma once

#ifndef GAL_LANG_COMPILER_COMPILER_HPP
#define GAL_LANG_COMPILER_COMPILER_HPP

#include <ast/parse_options.hpp>
#include <ast/parse_errors.hpp>
#include <builtin_name.h>

namespace gal
{
	namespace ast
	{
		class ast_statement_block;
		class ast_name_table;
	}

	namespace compiler
	{
		class bytecode_encoder;
		class bytecode_builder;

		/**
		 * @note this structure is duplicated in compile.h, don't forget to change these in sync!
		 */
		struct compile_options
		{
			// 0 - no optimization
			// 1 - baseline optimization level that doesn't prevent debug-ability
			// 2 - includes optimizations that harm debug-ability such as inlining
			int optimization_level;// default=1

			// 0 - no debugging support
			// 1 - line info & function names only; sufficient for back traces
			// 2 - full debug info with local & upvalue names; necessary for debugger
			int debug_level;// default=1

			// 0 - no code coverage support
			// 1 - statement coverage
			// 2 - statement and expression coverage (verbose)
			int coverage_level;// default=0

			// null-terminated array of globals that are mutable; disables the import optimization for fields accessed through these
			const char** mutable_globals;
		};

		using compile_error = ast::parse_error;

		// compiles bytecode into bytecode builder using either a pre-parsed AST or parsing it from source; throws on errors
		void compile_if_no_error(bytecode_builder& bytecode_builder, ast::ast_statement_block& root, const ast::ast_name_table& names, compile_options options = {});
		void compile_if_no_error(bytecode_builder& bytecode_builder, std::string_view source, compile_options compile_options = {}, ast::parse_options parse_options = {});

		// compiles bytecode into a bytecode blob, that either contains the valid bytecode or an encoded error that gal_load can decode
		std::string compile(std::string_view source, compile_options compile_options = {}, ast::parse_options parse_options = {}, bytecode_encoder* bytecode_encoder = nullptr);
	}
}

#endif
