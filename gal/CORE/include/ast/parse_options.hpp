#pragma once

#ifndef GAL_LANG_AST_PARSE_OPTIONS_HPP
	#define GAL_LANG_AST_PARSE_OPTIONS_HPP

namespace gal::ast
{
	enum class parse_mode
	{
		none,
		// unannotated symbols are any
		not_strict,
		// unannotated symbols are inferred
		strict,
		// type definition module, has special parsing rules
		definition
	};

	struct parse_options
	{
		bool allow_type_annotations = true;
		bool support_continue_statement = true;
		bool allow_declaration_syntax	= false;
		bool capture_comments			= false;
	};
}

#endif // GAL_LANG_AST_PARSE_OPTIONS_HPP
