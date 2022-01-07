#pragma once

#ifndef GAL_LANG_COMPILER_COMPILER_HPP
#define GAL_LANG_COMPILER_COMPILER_HPP

#include <ast/parse_options.hpp>
#include <compile_options.hpp>
#include <string_view>

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

		// compiles bytecode into bytecode builder using either a pre-parsed AST or parsing it from source; throws on errors
		void compile_if_no_error(bytecode_builder& bytecode_builder, ast::ast_statement_block& root, const ast::ast_name_table& names, compile_options options = {});
		void compile_if_no_error(bytecode_builder& bytecode_builder, std::string_view source, compile_options compile_options = {}, ast::parse_options parse_options = {});

		// compiles bytecode into a bytecode blob, that either contains the valid bytecode or an encoded error that gal_load can decode
		std::string compile(std::string_view source, compile_options compile_options = {}, ast::parse_options parse_options = {}, bytecode_encoder* bytecode_encoder = nullptr);
	}
}

#endif
