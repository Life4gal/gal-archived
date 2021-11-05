%skeleton "lalr1.cc"
%define api.parser.class { gal_parser }
%define api.token.prefix { gal_token_ }
%define api.token.constructor
// https://www.gnu.org/software/bison/manual/bison.html#C_002b_002b-Variants
%define api.value.type variant
%define parse.assert
%locations
%define parse.trace
%define parse.error
%define parse.lac full

// https://www.gnu.org/software/bison/manual/bison.html#g_t_0025code-Summary
%code requires
{
	// todo: add necessary files
	#include <string>
	#include <ast_node.hpp>
} //%code requires

%param { gal::context& context }//%param

%code
{
	namespace yy { gal_parser::symbol_type yylex(gal::context& context); }
}//%code

%token
		EOL
		RETURN "return"

		IF "if"
		ELSE "else"
		ELIF "elif"

		FOR "for"
		IN "in"

		WHILE "while"

		FROM "from"
		TO "to"

		NOT "not"
		IS "is"
		// typeof(value) get type of value
		TYPEOF "typeof"
		// put in global scope
		GLOBAL "global"
		// promote_as<value> -> promote as value's scope
		PROMOTE "promote_as"

		ARROW_LEFT "<-"
		ARROW_RIGHT "->"
		// the same as LESS_EQUAL
		// ROCKET_LEFT "<="
		ROCKET_RIGHT "=>"
		ROCKET_SHIP "<=>"
		PARENTHESES_OPEN "("
		PARENTHESES_CLOSE ")"
		SQUARE_BRACKET_OPEN "["
		SQUARE_BRACKET_CLOSE "]"
		CURLY_BRACKET_OPEN "{"
		CURLY_BRACKET_CLOSE "}"

		COMMA ","
		AND_EQUAL "&="
		XOR_EQUAL "^="
		OR_EQUAL "|="
		LEFT_SHIFT_EQUAL "<<="
		RIGHT_SHIFT_EQUAL "=>>"
		PRODUCT_EQUAL "*="
		QUOTIENT_EQUAL "/="
		REMAINDER_EQUAL "%="
		SUM "+="
		DIFFERENCE "-="
		ASSIGNMENT "="
		// for exception
		RAISE "RAISE"
		// ternary conditional
		QUESTION_MARK "?"
		COLON ":"
		LOGICAL_OR "||"
		LOGICAL_AND "&&"
		BITWISE_OR "|"
		BITWISE_XOR "^"
		BITWISE_AND "&"
		EQUAL "=="
		NOT_EQUAL "!="
		LESS_THAN "<"
		LESS_EQUAL "<="
		GREATER_THAN ">"
		GREATER_EQUAL ">="
		LEFT_SHIFT "<<"
		RIGHT_SHIFT ">>"
		// also represent as unary-plus which has higher precedence
		ADDITION "+"
		// also represent as unary-minus which has higher precedence
		SUBTRACTION "-"
		MULTIPLICATION "*"
		DIVISION "/"
		REMAINDER "%"
		BITWISE_NOT "~"

// the higher the line number of the declaration (lower on the page or screen), the higher the precedence
%left COMMA
%right AND_EQUAL XOR_EQUAL OR_EQUAL
%right LEFT_SHIFT_EQUAL RIGHT_SHIFT_EQUAL
%right PRODUCT_EQUAL QUOTIENT_EQUAL REMAINDER_EQUAL
%right SUM DIFFERENCE ASSIGNMENT
%right QUESTION_MARK COLON
%left LOGICAL_OR LOGICAL_AND LOGICAL_AND
%left BITWISE_OR BITWISE_AND BITWISE_AND
%left EQUAL NOT_EQUAL
%left LESS_THAN LESS_EQUAL GREATER_THAN GREATER_EQUAL
%left LEFT_SHIFT RIGHT_SHIFT
// need to determine whether it is a unary operator, unary operators' associativity is right-to-left
%left ADDITION SUBTRACTION
%left MULTIPLICATION DIVISION REMAINDER
%left BITWISE_NOT

%type<gal::integer_type> INTEGER
%type<gal::number_type> NUMBER
%type<gal::string_type> STRING
%type<gal::boolean_type> BOOLEAN
%type<gal::identifier_type> IDENTIFIER e_identifier

%type<gal::expression_type> e_expressions e_expression
// { expressions } [ expressions ] ( expressions )
%type<gal::expression_type> e_bracketed_expression
// expression1, expression2, expression3 ...
%type<gal::expression_type> e_comma_expression
%type<gal::expression_type> expression

%type<gal::expression_type> e_statement
%type<gal::expression_type> statement



%%

library:
	{
		// build current local scope
		// todo: better scope manager
		[[maybe_unused]] auto scope = gal::make_expression<gal::ast_scope>("global scope");
	}
	functions
	{
		/* scope destructed, scope finished */
	};

e_parentheses_close:
	error{}
	|
	// expected ')'
	PARENTHESES_CLOSE;

e_square_bracket_close:
	error{}
	|
	// expected ']'
	SQUARE_BRACKET_CLOSE;

e_curly_bracket_close:
	error{}
	|
	CURLY_BRACKET_CLOSE;

functions:
	/* nothing here */
	%empty
	|
	functions
	/* `[`FUNCTION_NAME`]` `=>` (PARAMETERS) `=>` RETURN_TYPE `:` e_statement */
	SQUARE_BRACKET_OPEN e_identifier e_square_bracket_close ROCKET_RIGHT
	{
		// build current local scope
		// todo: add global scope as parent
        [[maybe_unused]] auto scope = gal::make_expression<gal::ast_scope>("scope-" + $3);
	}
	PARENTHESES_OPEN parameter_declarations e_parentheses_close function_return
	e_statement
	{
		// pass -> function_name, parameters, return_type, function_body
		context.add_function(std::move($3), std::move($7), std::move($9), std::move($10));
		/* scope destructed, scope finished */
	};

e_identifier:
	error{}
	|
	IDENTIFIER
	{
		$$ = std::move($1);
	};

parameter_declarations:
	%empty
	|
	parameter_declaration;

parameter_declaration:
	// only one parameter
	e_identifier
	{
		$$ = gal::make_expression<gal::ast_args_pack>();
		$$.push_arg(std::move($1));
	}
	// parameter1, parameter2, parameter3 ...
	parameter_declaration COMMA e_identifier
	{
		$$ = std::move($1);
		// push parameters
		$$.push_arg(std::move($3));
	};

function_return:
	%empty
	|
	ROCKET_RIGHT IDENTIFIER
	{
		$$ = std::move($2);
	};

e_expressions:
	error{}
	|
	expressions
	{
		$$ = std::move($1);
	};

e_expression:
	error{}
	|
	expression
	{
		$$ = std::move($1);
	}

e_bracketed_expression:
	error{}
	|
	PARENTHESES_BRACKET_OPEN e_expressions e_parentheses_close
	{
		$$ = std::move($2);
	}
	|
	SQUARE_BRACKET_OPEN e_expressions e_square_bracket_close
	{
    	$$ = std::move($2);
    }
	|
	CURLY_BRACKET_OPEN e_expressions e_curly_bracket_close
	{
    	$$ = std::move($2);
    };

e_comma_expression:
	e_expression
	{
		$$ = gal::make_expression<gal::ast_prototype>(std::move($1));
	}
	|
	e_comma_expression COMMA e_expression
	{
		$$ = std::move($1);
		$$.push_arg(std::move($3));
	};

e_statement:
	error{}
	|
	statement
	{
		$$ = std::move($1);
	};

e_if_statement:
	error{}
	|
	if_statement
	{
		$$ = std::move($1);
	};

else_statement:
	%empty
	|
	// else: balabala
	ELSE COLON e_statement
	{
		$$ = gal::make_expression<ast_else_expr>(std::move($3));
	};

if_statement:
	if_statement
	// if xxx: balabala
	IF e_expression COLON e_statement
	{
		$$ = gal::make_expression<ast_if_expr>(std::move($5), std::move($3));
	}
	|
	// elif xxx: balabala
	ELIF e_expression COLON e_statement
	{
		$$.add_branch(std::move(gal::make_expression<ast_if_expr>(std::move($4), std::move($2));
	}
	|
	else_statement
	{
		$$.add_branch(std::move($1));
	};

statement:
    // todo

%%














