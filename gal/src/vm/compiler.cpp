#include <vm/compiler.hpp>
#include <string_view>
#include <vector>
#include <array>
#include <utils/format.hpp>
#include <vm/common.hpp>
#include <vm/vm.hpp>
#include <vm/debugger.hpp>
#include <gal.hpp>
#include <utils/utils.hpp>

// todo: use bison

namespace gal
{
	/**
	 * @brief The maximum number of local (i.e. not module level) variables that can be
	 * declared in a single function, method, or chunk of top level code. This is
	 * the maximum number of variables in scope at one time, and spans block scopes.
	 *
	 * Note that this limitation is also explicit in the byte-code. Since
	 * `CODE_LOAD_LOCAL` and `CODE_STORE_LOCAL` use a single argument byte to
	 * identify the local, only 256 can be in scope at one time.
	 */
	constexpr auto max_locals = 256;

	/**
	 * @brief The maximum number of upvalues (i.e. variables from enclosing functions)
	 * that a function can close over.
	 */
	constexpr auto max_upvalues = 256;

	/**
	 * @brief The maximum number of distinct constants that a function can contain. This
	 * value is explicit in the byte-code since `CODE_CONSTANT` only takes a single
	 * two-byte argument.
	 */
	constexpr auto max_constants = 1 << 16;

	/**
	 * @brief The maximum distance a CODE_JUMP or CODE_JUMP_IF instruction can move the
	 *  instruction pointer
	 */
	constexpr auto max_jump = 1 << 16;

	namespace
	{
		constexpr bool is_uppercase_char(const char c) { return c >= 'A' && c <= 'Z'; }

		constexpr bool is_lowercase_char(const char c) { return c >= 'a' && c <= 'z'; }

		constexpr bool is_name(const char c) { return is_uppercase_char(c) || is_lowercase_char(c) || c == '_'; }

		constexpr bool is_digit(const char c) { return c >= '0' && c <= '9'; }

		constexpr int to_digit(const char c)
		{
			if (is_uppercase_char(c)) { return c - 'A' + 10; }
			if (is_lowercase_char(c)) { return c - 'a' + 10; }
			if (is_digit(c)) { return c - '0'; }
			return -1;
		}
	}

	enum class gal_token_type
	{
		// "<-"
		ARROW_LEFT,
		// "->"
		ARROW_RIGHT,
		// the same as LESS_EQUAL
		// "<="
		// ROCKET_LEFT,
		// "=>"
		ROCKET_RIGHT,
		// "<=>"
		ROCKET_SHIP,
		// "("
		PARENTHESES_OPEN,
		// ")"
		PARENTHESES_CLOSE,
		// "["
		SQUARE_BRACKET_OPEN,
		// "]"
		SQUARE_BRACKET_CLOSE,
		// "{"
		CURLY_BRACKET_OPEN,
		// "}"
		CURLY_BRACKET_CLOSE,
		// ","
		COMMA,
		// "&="
		AND_EQUAL,
		// "^="
		XOR_EQUAL,
		// "|="
		OR_EQUAL,
		// "<<="
		LEFT_SHIFT_EQUAL,
		// "=>>"
		RIGHT_SHIFT_EQUAL,
		// "*="
		PRODUCT_EQUAL,
		// "/="
		QUOTIENT_EQUAL,
		// "%="
		REMAINDER_EQUAL,
		// "+="
		SUM,
		// "-="
		DIFFERENCE,
		// "="
		ASSIGNMENT,
		// ":"
		COLON,
		// "||"
		LOGICAL_OR,
		// "&&"
		LOGICAL_AND,
		// "|"
		BITWISE_OR,
		// "^"
		BITWISE_XOR,
		// "&"
		BITWISE_AND,
		// "=="
		EQUAL,
		// "!="
		NOT_EQUAL,
		// "<"
		LESS_THAN,
		// "<="
		LESS_EQUAL,
		// ">"
		GREATER_THAN,
		// ">="
		GREATER_EQUAL,
		// "<<"
		LEFT_SHIFT,
		// ">>"
		RIGHT_SHIFT,
		// also represent as unary-plus which has higher precedence
		// "+"
		PLUS,
		// also represent as unary-minus which has higher precedence
		// "-"
		MINUS,
		// "*"
		MULTIPLY,
		// "/"
		DIVIDE,
		// "%"
		MODULUS,
		// "~"
		BITWISE_NOT,

		AS,
		IMPORT,
		OUTER,

		LOOP_WHILE,
		LOOP_FOR,
		LOOP_BREAK,
		LOOP_CONTINUE,

		BRANCH_IF,
		BRANCH_ELSE,
		BRANCH_ELSE_IF,

		CLASS_DECL,
		CLASS_CTOR,
		CLASS_THIS,
		CLASS_SUPER,
		CLASS_STATIC,
		CLASS_INHERIT,
		CLASS_FIELD,
		CLASS_STATIC_FIELD,

		BOOLEAN_TRUE,
		BOOLEAN_FALSE,

		IDENTIFIER_NAME,
		IDENTIFIER_NUMBER,
		IDENTIFIER_STRING,
		IDENTIFIER_VAR,
		IDENTIFIER_NULL,

		STATEMENT_RETURN,
		STATEMENT_NEW_LINE,
		STATEMENT_ERROR,
		STATEMENT_EOF,
	};

	struct gal_keyword
	{
		std::string_view identifier;
		gal_token_type type;
	};

	namespace
	{
		constexpr gal_keyword keywords[] =
		{
				{"as", gal_token_type::AS},
				{"import", gal_token_type::IMPORT},
				{"outer", gal_token_type::OUTER},

				{"while", gal_token_type::LOOP_WHILE},
				{"for", gal_token_type::LOOP_FOR},
				{"break", gal_token_type::LOOP_BREAK},
				{"continue", gal_token_type::LOOP_CONTINUE},

				{"if", gal_token_type::BRANCH_IF},
				{"else", gal_token_type::BRANCH_ELSE},
				{"elif", gal_token_type::BRANCH_ELSE_IF},

				{"class", gal_token_type::CLASS_DECL},
				{"construct", gal_token_type::CLASS_CTOR},
				{"this", gal_token_type::CLASS_THIS},
				{"super", gal_token_type::CLASS_SUPER},
				{"static", gal_token_type::CLASS_STATIC},
				{"extend", gal_token_type::CLASS_INHERIT},

				{"True", gal_token_type::BOOLEAN_TRUE},
				{"False", gal_token_type::BOOLEAN_FALSE},

				{"var", gal_token_type::IDENTIFIER_VAR},
				{"Null", gal_token_type::IDENTIFIER_NULL},

				{"return", gal_token_type::STATEMENT_RETURN},
				{"", gal_token_type::STATEMENT_EOF}};
	}

	struct gal_token
	{
		gal_token_type type;

		// The token, pointing directly into the source.
		std::string_view identifier;

		// The 1-based line where the token appears.
		int line;

		// The parsed value if the token is a literal.
		magic_value value;
	};

	struct gal_parser
	{
		gal_virtual_machine_state& state;

		// @brief The module being parsed. 
		object_module& current_module;

		// The source code being parsed.
		const char* source;

		// The beginning of the currently-being-lexed token in [source].
		const char* token_begin;

		// The current character being lexed in [source].
		const char* current_char;

		// The 1-based line number of [current_char].
		int current_line;

		// The most recently consumed/advanced token.
		gal_token token_previous;

		// The most recently lexed token.
		gal_token token_current;

		// The upcoming token.
		gal_token token_next;

		// Whether compile errors should be printed to stderr or discarded.
		bool print_errors;

		// If a syntax or compile error has occurred.
		bool has_error;

		[[nodiscard]] char peek() const noexcept { return current_char[0]; }

		[[nodiscard]] char peek_previous() const noexcept { return current_char[-1]; }

		[[nodiscard]] char peek_next() const noexcept
		{
			// If we're at the end of the source, don't read past it.
			if (peek() == '\0') return '\0';
			return current_char[1];
		}

		char next() noexcept
		{
			const auto ret = peek();
			++current_char;
			if (ret == '\n') { ++current_line; }
			return ret;
		}

		[[nodiscard]] bool advance_if(const char c)
		{
			if (peek() == c)
			{
				next();
				return true;
			}
			return false;
		}

		template<typename Pred>
		[[nodiscard]] bool advance_if(Pred pred)
		{
			if (pred(peek()))
			{
				next();
				return true;
			}
			return false;
		}

		template<char... Cs>
		[[nodiscard]] bool contain() const noexcept { return ((peek() == Cs) || ...); }

		template<typename... Preds>
			requires (std::is_invocable_r_v<bool, Preds, char> && ...)
		[[nodiscard]] bool contain(Preds ... preds) { return (preds(peek()) || ...); }

		void make_token(const gal_token_type type)
		{
			token_next.type = type;
			token_next.identifier = {token_begin, static_cast<std::string_view::size_type>(current_char - token_begin)};
			token_next.line = current_line;

			// Make line tokens appear on the line containing the "\n".
			if (type == gal_token_type::STATEMENT_NEW_LINE) { --token_next.line; }
		}

		void match_make_token(const char c, const gal_token_type match_type, const gal_token_type not_match_type) { make_token(advance_if(c) ? match_type : not_match_type); }

		template<std::pair<char, gal_token_type>... Pairs>
		bool match_make_token()
		{
			return ([&](const auto& pair)
			{
				if (advance_if(pair.first))
				{
					make_token(pair.second);
					return true;
				}
				return false;
			}(Pairs) || ...);
		}

		void skip_line_comment() { while (not contain<'\n', '\0'>()) { next(); } }

		void skip_block_comment()
		{
			int nesting = 1;
			while (nesting > 0)
			{
				if (peek() == '\0')
				{
					lex_error("Unterminated block comment.");
					return;
				}

				//
				//	/* block comment
				//		block comment
				//		block comment
				//		block comment
				//		block comment
				//	*/
				//
				if (peek() == '/' && peek_next() == '*')
				{
					next();
					next();
					++nesting;
					continue;
				}

				if (peek() == '*' && peek_next() == '/')
				{
					next();
					next();
					--nesting;
					continue;
				}

				// Regular comment character.
				next();
			}
		}

		/**
		 * @brief Reads the next character, which should be a hex digit (0-9, a-f, or A-F) and
		 * returns its numeric value. If the character isn't a hex digit, returns -1.
		 */
		int read_hex_digit()
		{
			const auto c = next();
			if (const auto result = to_digit(c); result != -1) { return result; }
			else
			{
				// Don't consume it if it isn't expected. Keeps us from reading past the end
				// of an unterminated string.
				--current_char;
				return result;
			}
		}

		/**
		 * @brief Parses the numeric value of the current token.
		 */
		template<bool IsHex>
		void make_number()
		{
			// todo: handle too large number
			if constexpr (IsHex)
			{
				magic_value::value_type value;
				std::from_chars(token_begin, nullptr, value, 16);
				token_next.value = magic_value{value};
			}
			else
			{
				double value;
				std::from_chars(token_begin, nullptr, value);
				token_next.value = magic_value{value};
			}

			make_token(gal_token_type::IDENTIFIER_NUMBER);
		}

		/**
		 * @brief Finishes lexing a hexadecimal number literal.
		 */
		void read_hex_number()
		{
			// Skip past the `x` used to denote a hexadecimal literal.
			next();

			// Iterate over all the valid hexadecimal digits found.
			while (read_hex_digit() != -1) { }

			make_number<true>();
		}

		/**
		 * @brief Finishes lexing a number literal.
		 */
		void read_number()
		{
			while (advance_if(is_digit)) {}

			// See if it has a floating point. Make sure there is a digit after the "."
			// so we don't get confused by method calls on number literals.
			if (peek() == '.' && is_digit(peek_next()))
			{
				next();
				while (advance_if(is_digit)) {}
			}

			// See if the number is in scientific notation.
			if (advance_if([](const char c) { return c == 'e' || c == 'E'; }))
			{
				// Allow a single positive/negative exponent symbol.
				if (not advance_if('+')) { (void)advance_if('-'); }

				if (not is_digit(peek())) { lex_error("Unterminated scientific notation."); }

				while (advance_if(is_digit)) {}
			}

			make_number<false>();
		}

		/**
		 * @brief Finishes lexing an identifier. Handles reserved words.
		 */
		void read_name(gal_token_type type, const char first_char)
		{
			const auto string = object::create<object_string>(1, first_char);

			while (contain(is_name, is_digit)) { string->push_back(next()); }

			// Update the type if it's a keyword.
			const std::string_view current{token_begin, static_cast<std::string_view::size_type>(current_char - token_begin)};
			if (const auto it = std::ranges::find(keywords, current, [](const auto& keyword)
												  { return keyword.identifier; }); it != std::end(keywords))
			{
				type = it->type;
			}

			token_next.value = string->operator magic_value();
			make_token(type);
		}

		/**
		 * @brief Reads [digits] hex digits in a string literal and returns their number value.
		 */
		int read_hex_escape(const std::size_t digits, const char* description)
		{
			int value = 0;
			for (std::size_t i = 0; i < digits; ++i)
			{
				if (const char c = peek(); c == '"' || c == '\0')
				{
					lex_error(std_format::format("Incomplete {} escape sequence.", description));

					// Don't consume it if it isn't expected. Keeps us from reading past the
					// end of an unterminated string.
					--current_char;
					break;
				}

				if (const int digit = read_hex_digit(); digit == -1)
				{
					lex_error(std_format::format("Invalid {} escape sequence.", description));
					break;
				}
				else { value = (value * 16) | digit; }
			}

			return value;
		}

		/**
		 * @brief Reads a hex digit Unicode escape sequence in a string literal.
		 */
		void read_unicode_escape(const std::size_t length, object_string& out_string)
		{
			const auto value = read_hex_escape(length, "Unicode");

			// Grow the buffer enough for the encoded result.
			if (const auto num_bytes = utf8_encode_num_bytes(value); num_bytes != 0) { utf8_encode(value, out_string.get_appender()); }
		}

		void read_raw_string()
		{
			const auto string = object::create<object_string>();
			constexpr auto type = gal_token_type::IDENTIFIER_STRING;

			while (true)
			{
				const auto c = next();
				if (c == '"') { break; }
				if (c == '\r') { continue; }
				if (c == '\0')
				{
					lex_error("Unterminated string.");

					// Don't consume it if it isn't expected. Keeps us from reading past the
					// end of an unterminated string.
					--current_char;
					break;
				}

				// todo: more string interface

				if (c == '\\')
				{
					switch (next())
					{
						case '"':
						case '\\':
						case '%':
						{
							string->push_back(c);
							break;
						}
						case '0':
						{
							string->push_back('\0');
							break;
						}
						case 'a':
						{
							string->push_back('\a');
							break;
						}
						case 'b':
						{
							string->push_back('\b');
							break;
						}
						case 'e':
						{
							string->push_back('\33');
							break;
						}
						case 'f':
						{
							string->push_back('\f');
							break;
						}
						case 'n':
						{
							string->push_back('\n');
							break;
						}
						case 'r':
						{
							string->push_back('\r');
							break;
						}
						case 't':
						{
							string->push_back('\t');
							break;
						}
						case 'u':
						{
							read_unicode_escape(4, *string);
							break;
						}
						case 'U':
						{
							read_unicode_escape(8, *string);
							break;
						}
						case 'v':
						{
							string->push_back('\v');
							break;
						}
						case 'x':
						{
							string->push_back(static_cast<object_string::value_type>(read_hex_escape(2, "byte")));
							break;
						}
						default:
						{
							lex_error(std_format::format("Invalid escape character {}.", peek_previous()));
							break;
						}
					}
				}
				else { string->push_back(c); }
			}

			token_next.value = string->operator magic_value();

			make_token(type);
		}

		void next_token()
		{
			token_previous = token_current;
			token_current = token_next;

			// If we are out of tokens, don't try to tokenize any more. We *do* still
			// copy the TOKEN_EOF to previous so that code that expects it to be consumed
			// will still work.
			if (token_next.type == gal_token_type::STATEMENT_EOF) { return; }
			if (token_current.type == gal_token_type::STATEMENT_EOF) { return; }

			while (peek() != '\0')
			{
				token_begin = current_char;

				switch (const auto c = next())
				{
					case '(':
					{
						make_token(gal_token_type::PARENTHESES_OPEN);
						break;
					}
					case ')':
					{
						make_token(gal_token_type::PARENTHESES_CLOSE);
						break;
					}
					case '[':
					{
						make_token(gal_token_type::SQUARE_BRACKET_OPEN);
						break;
					}
					case ']':
					{
						make_token(gal_token_type::SQUARE_BRACKET_CLOSE);
						break;
					}
					case '{':
					{
						make_token(gal_token_type::CURLY_BRACKET_OPEN);
						break;
					}
					case '}':
					{
						make_token(gal_token_type::CURLY_BRACKET_CLOSE);
						break;
					}
					case ',':
					{
						make_token(gal_token_type::COMMA);
						break;
					}
					case '&':
					{
						if (not match_make_token<std::make_pair('=', gal_token_type::AND_EQUAL), std::make_pair('&', gal_token_type::LOGICAL_AND)>()) { make_token(gal_token_type::BITWISE_AND); }
						break;
					}
					// todo
				}
			}
		}

		void print_error(const int line, const std::string_view reason, const std::string_view detail)
		{
			has_error = true;
			if (not print_errors) { return; }

			// Only report errors if there is a standard_error_handler_function to handle them.
			if (const auto handler = state.configuration_.standard_error_handler_function; handler) { handler(state, gal_error_type::ERROR_COMPILE, current_module.get_name().str(), line, reason, detail); }
		}

		void lex_error(const std::string_view detail) { print_error(current_line, "Lex error", detail); }

		void compile_error(const std::string_view detail)
		{
			// If the parse error was caused by an error token, the lexer has already
			// reported it.
			if (token_previous.type == gal_token_type::STATEMENT_ERROR) { return; }

			if (token_previous.type == gal_token_type::STATEMENT_NEW_LINE) { print_error(token_previous.line, "Error at new line", detail); }
			else if (token_previous.type == gal_token_type::STATEMENT_EOF) { print_error(token_previous.line, "Error at end of file", detail); }
			else { print_error(token_previous.line, std_format::format("Error at '{}'", token_previous.identifier), detail); }
		}
	};

	struct gal_local
	{
		// The name of the local variable. This points directly into the original
		// source code string.
		std::string_view variable_identifier;

		// The depth in the scope chain that this variable was declared at. Zero is
		// the outermost scope--parameters for a method, or the first local block in
		// top level code. One is the scope within that, etc.
		int depth;

		// If this local variable is being used as an upvalue.
		bool is_upvalue;
	};

	struct gal_compiler_upvalue
	{
		// True if this upvalue is capturing a local variable from the enclosing
		// function. False if it's capturing an upvalue.
		bool is_local;

		// The index of the local or upvalue being captured in the enclosing function.
		int index;
	};

	/**
	 * @brief Bookkeeping information for the current loop being compiled.
	 */
	struct gal_loop
	{
		// Index of the instruction that the loop should jump back to.
		int start;

		// Index of the argument for the CODE_JUMP_IF instruction used to exit the
		// loop. Stored so we can patch it once we know where the loop ends.
		int exit_jump;

		// Index of the first instruction of the body of the loop.
		int body;

		// Depth of the scope(s) that need to be exited if a break is hit inside the
		// loop.
		int scope_depth;

		// The loop enclosing this one, or nullptr if this is the outermost loop.
		gal_loop* next;
	};

	/**
	 * @brief The different signature syntax for different kinds of methods.
	 */
	enum class gal_signature_type
	{
		method,

		getter,

		setter,

		ctor,
	};

	struct gal_signature
	{
		std::string_view name;
		gal_signature_type type;
		int arity;
	};

	/**
	 * @brief Bookkeeping information for compiling a class definition.
	 */
	struct gal_class_info
	{
		// The name of the class.
		object_string name;

		// Attributes for the class itself
		object_map class_attributes;

		// Attributes for methods in this class
		object_map method_attributes;

		// Symbol table for the fields of the class.
		symbol_table fields;

		// Symbols for the methods defined by the class. Used to detect duplicate
		// method definitions.
		std::vector<int> methods;
		std::vector<int> static_methods;

		// True if the class being compiled is a outer class.
		bool is_outer;

		// True if the current method being compiled is static.
		bool in_static;

		// The signature of the method being compiled.
		gal_signature sig;
	};

	class gal_compiler
	{
		gal_parser& parser_;

		// The compiler for the function enclosing this one, or nullptr if it's the
		// top level.
		gal_compiler* parent_;

		// The currently in scope local variables.
		std::vector<gal_local> locals_;

		// The upvalues that this function has captured from outer scopes. The count
		// of them is stored in [function->num_upvalues].
		std::vector<gal_compiler_upvalue> upvalues_;

		// The current level of block scope nesting, where zero is no nesting. A -1
		// here means top-level code is being compiled and there is no block scope
		// in effect at all. Any variables declared will be module-level.
		int scope_depth_;

		// The current number of slots (locals and temporaries) in use.
		//
		// We use this and [function->max_slots] to track the maximum number of
		// additional slots a function may need while executing. When the function
		// is called, the fiber will check to ensure its stack has enough room to cover
		// that worst case and grow the stack if needed.
		//
		// This value here doesn't include parameters to the function. Since those
		// are already pushed onto the stack by the caller and tracked there, we
		// don't need to double count them here.
		gal_size_type num_slots_;

		// The current innermost loop being compiled, or nullptr if not in a loop.
		gal_loop* loop_;

		// If this is a compiler for a method, keeps track of the class enclosing it.
		gal_class_info enclosing_class_;

		// The function being compiled.
		object_function function_;

		// The constants for the function being compiled.
		object_map constants_;

		// Whether or not the compiler is for a constructor initializer
		bool is_ctor_;

		// The number of attributes seen while parsing.
		// We track this separately as compile time attributes
		// are not stored, so we can't rely on attributes->size
		// to enforce an error message when attributes are used
		// anywhere other than methods or classes.
		gal_size_type num_attributes_;

		// Attributes for the next class or method.
		object_map attributes_;

		gal_compiler(gal_parser& parser, gal_compiler* parent, bool is_method)
			: parser_{parser},
			  parent_{parent},
			  locals_(1, gal_local{is_method ? "this" : nullptr, -1, false}),
			  scope_depth_{
					  parent
						  ?
						  // The initial scope for functions and methods is local scope.
						  0
						  :
						  // Compiling top-level code, so the initial scope is module-level.
						  -1},
			  // Declare a local slot for either the closure or method receiver so that we
			  // don't try to reuse that slot for a user-defined local variable. For
			  // methods, we name it "this", so that we can resolve references to that like
			  // a normal variable. For functions, they have no explicit "this", so we use
			  // an empty name. That way references to "this" inside a function walks up
			  // the parent chain to find a method enclosing the function whose "this" we
			  // can close over.
			  num_slots_{locals_.size()},
			  loop_{nullptr},
			  enclosing_class_{},
			  function_{parser.current_module, 1},
			  is_ctor_{false},
			  num_attributes_{0} { }

		/**
		 * @brief Outputs a compile or syntax error. This also marks the compilation as having
		 * an error, which ensures that the resulting code will be discarded and never
		 * run. This means that after calling error(), it's fine to generate whatever
		 * invalid byte-code you want since it won't be used.
		 *
		 * @note Most places that call error() continue to parse and compile
		 * after that. That's so that we can try to find as many compilation errors in
		 * one pass as possible instead of just bailing at the first one.
		 */
		void error(const std::string_view detail) const { parser_.compile_error(detail); }

		/**
		 * @brief Adds [constant] to the constant pool and returns its index.
		 */
		gal_index_type add_constant(const magic_value constant)
		{
			if (parser_.has_error) { return gal_index_not_exist; }

			// See if we already have a constant for the value. If so, reuse it.
			if (const auto existing = constants_.get(constant); existing.is_number()) { return static_cast<gal_index_type>(existing.as_number()); }

			// It's a new constant.
			if (function_.get_constant_size() < max_constants)
			{
				const auto index = function_.add_constant_get(constant);
				constants_.set(constant.weak_this(), magic_value{index});
				return index;
			}

			error(std_format::format("A function may only contain {} unique constants.", max_constants));
			return gal_index_not_exist;
		}

		void disallow_attributes();
		void add_to_attribute_group(magic_value group, magic_value key, magic_value value);
		void emit_class_attributes(gal_class_info& class_info);
		void copy_attributes(object_map& target);
		void copy_method_attributes(bool is_outer, bool is_static, std::string_view full_signature);
	};

	/**
	 * @brief Describes where a variable is declared.
	 */
	enum class gal_scope_type
	{
		// A local variable in the current function.
		local_level,

		// A local variable declared in an enclosing function.
		upvalue_level,

		// A top-level module variable.
		module_level,
	};

	/**
	 * @brief A reference to a variable and the scope where it is defined. This contains
	 *  enough information to emit correct code to load or store the variable.
	 */
	struct gal_variable
	{
		// The stack slot, upvalue slot, or module symbol defining the variable.
		int index;

		// Where the variable is declared.
		gal_scope_type scope;
	};

	namespace
	{
		/**
		 * @brief The stack effect of each opcodes. The index in the array is
		 * the opcodes, and the value is the stack effect of that instruction.
		 */
		constexpr int stack_effects[] =
		{
				#define OPCODE(_, effect) effect
				#include "vm/opcodes.config"
				#undef OPCODE
		};
	}
}
