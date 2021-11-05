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
} //%code requires

%param { gal::lexer::context& context }//%param

%code
{
	namespace yy { gal_parser::symbol_type yylex(gal::lexer::context& context); }
}//%code

%token
		EOL
		RETURN "return"

		IF "if"
		ELSE "else"

		FOR "for"
		WHILE "while"
		IN "in"

		DEF "def"

		ARROW_LEFT "<-"
		ARROW_RIGHT "->"
		// the same as LESS_EQUAL
		// ROCKET_LEFT "<="
		ROCKET_RIGHT "=>"
		ROCKET_SHIP "<=>"
		PARENTHESE_OPEN "("
		PARENTHESE_CLOSE ")"
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
		NOT_EQAUL "!="
		LESS_THAN "<"
		LESS_EQUAL "<="
		GREATER_THAN ">"
		GREATER_EQUAL ">="
		LEFT_SHIFT "<<"
		RIGHT_SHIFT ">>"
		// also represent as unary-plus
		ADDITION "+"
		// also represent as unary-minus
		SUBSTRACTION "-"
		MULTIPLICATION "*"
		DIVISION "/"
		REMAINDER "%"

// the higher the line number of the declaration (lower on the page or screen), the higher the precedence
%left COMMA
%right AND_EQUAL XOR_EQUAL OR_EQUAL
%right LEFT_SHIFT_EQUAL RIGHT_SHIFT_EQUAL
%right PRODUCT_EQUAL QUOTIENT_EQUAL REMAINDER_EQUAL
%right SUM DIFFERENCE ASSIGNMENT
%right QUESTION_MARK COLON
%left LOGICAL_OR LOGICAL_AND LOGICAL_AND
%left BITWISE_OR BITWISE_AND BITWISE_AND
%left EQUAL NOT_EQAUL
%left LESS_THAN LESS_EQUAL GREATER_THAN GREATER_EQUAL
%left LEFT_SHIFT RIGHT_SHIFT
// need to determine whether it is a unary operator, unary operators' associativity is right-to-left
%left ADDITION SUBSTRACTION
%left MULTIPLICATION DIVISION

%token<gal::integer_type> INTEGER
%token<gal::number_type> NUMBER
%token<gal::string_type> STRING
%token<gal::boolean_type> BOOLEAN
%token<gal::identifier_type> IDENTIFIER i_identifier

%%

library: { [[maybe_unused]] auto scope = context.new_scope(); } functions { /* s destruct, scope finished */ };
functions:
	/* nothing here */
	%empty
	|
	functions // todo

%%














