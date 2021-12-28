#pragma once

#ifndef GAL_LANG_AST_PARSER_HPP
#define GAL_LANG_AST_PARSER_HPP

#include <ast/ast.hpp>
#include <ast/lexer.hpp>
#include <ast/parse_options.hpp>
#include <ast/parse_errors.hpp>
#include <utils/string_utils.hpp>
#include <utils/allocator.hpp>

#include <vector>
#include <stack>

namespace gal::ast
{
	// template<typename T>
	// using ast_stack = std::stack<T, std::vector<T>>;

	template<typename T>
	class temporary_stack
	{
		template<typename> friend class temporary_stack;
	public:
		using holding_container_type = std::vector<T>;
		using size_type = typename holding_container_type::size_type;
		using value_type = typename holding_container_type::value_type;

	private:
		holding_container_type& container_;
		size_type begin_;
		size_type used_;

	public:
		explicit temporary_stack(holding_container_type& container)
			: container_{container},
			  begin_{container.size()},
			  used_{0} {}

		temporary_stack(const temporary_stack&) = delete;
		temporary_stack& operator=(const temporary_stack&) = delete;
		temporary_stack(temporary_stack&&) = delete;
		temporary_stack& operator=(temporary_stack&&) = delete;

		~temporary_stack() noexcept { container_.erase(begin(), end()); }

		[[nodiscard]] constexpr decltype(auto) top() const { return std::as_const(container_.back()); }

		[[nodiscard]] constexpr decltype(auto) bottom() const { return std::as_const(container_[begin_]); }

		[[nodiscard]] constexpr bool empty() const noexcept { return used_ == 0; }

		[[nodiscard]] constexpr size_type size() const noexcept { return used_; }

		constexpr decltype(auto) operator[](size_type index) { return container_[begin_ + index]; }

		constexpr void push(const value_type& value)
		{
			++used_;
			container_.push_back(value);
		}

		constexpr void push(value_type&& value)
		{
			++used_;
			container_.push_back(std::move(value));
		}

		template<typename... Args>
		constexpr decltype(auto) emplace(Args&&... args)
		{
			++used_;
			return container_.emplace_back(std::forward<Args>(args)...);
		}

		constexpr void pop()
		{
			--used_;
			container_.pop_back();
		}

		template<typename U, typename Func>
			requires requires(temporary_stack<T>& dest, const temporary_stack<U>& source, Func func)
			{
				dest.push(func(source.top()));
			}
		constexpr void insert(const temporary_stack<U>& source, Func func) { for (const auto& data: source) { push(func(data)); } }

		template<std::convertible_to<T> U>
		constexpr void insert(const temporary_stack<U>& source) { for (const auto& data: source) { push(static_cast<T>(data)); } }

	private:
		// internal use only
		[[nodiscard]] constexpr auto begin() noexcept
		{
			return container_.begin() + begin_;
		}
		[[nodiscard]] constexpr auto begin() const noexcept
		{
			return container_.cbegin() + begin_;
		}
		[[nodiscard]] GAL_ASSERT_CONSTEXPR auto end() noexcept
		{
			gal_assert(container_.size() == begin_ + used_);
			return container_.end();
		}
		[[nodiscard]] GAL_ASSERT_CONSTEXPR auto end() const noexcept
		{
			gal_assert(container_.size() == begin_ + used_);
			return container_.cend();
		}
	};

	struct parse_result
	{
		using parse_errors_type = parse_errors::parse_errors_container_type;
		using comment_locations_type = std::vector<comment>;

		ast_statement_block* root;
		std::vector<std::string> hot_comments;
		parse_errors_type errors;

		comment_locations_type comment_locations;
	};

	class parser
	{
	public:
		using ast_statement_stack_type = std::vector<ast_statement*>;
		using ast_expression_stack_type = std::vector<ast_expression*>;
		using ast_name_stack_type = std::vector<ast_name>;
		using ast_local_stack_type = std::vector<ast_local*>;
		using ast_table_property_stack_type = std::vector<ast_type_table::ast_table_property>;
		using ast_type_stack_type = std::vector<ast_type*>;
		using ast_type_or_pack_type = std::vector<ast_type_or_pack>;
		using ast_class_property_stack_type = std::vector<ast_statement_declare_class::ast_declared_class_property>;
		using ast_table_item_stack_type = std::vector<ast_expression_table::item>;
		using ast_argument_name_stack_type = std::vector<ast_argument_name>;
		using ast_optional_argument_name_stack_type = std::vector<std::optional<ast_argument_name>>;

		using locals_stack_size_type = ast_local_stack_type::size_type;

	private:
		struct parse_name_result
		{
			ast_name name;
			utils::location loc;
		};

		struct parse_name_binding_result
		{
			parse_name_result name;
			ast_type* annotation;
		};

		struct parse_local_result
		{
			ast_local* local;
			lexer::offset_type offset;
		};

		struct parse_function_result
		{
			bool vararg;
			std::size_t loop_depth;

			[[nodiscard]] constexpr bool is_root() const noexcept { return loop_depth == 0; }
		};

		using parse_name_binding_result_stack_type = std::vector<parse_name_binding_result>;
		using parse_function_result_stack_type = std::vector<parse_function_result>;

		parse_options options_;

		lexer lexer_;
		utils::trivial_allocator& allocator_;

		parse_result::comment_locations_type comment_locations_;

		std::size_t recursion_counter_;

		ast_name name_self_;
		ast_name name_number_;
		ast_name name_error_;
		ast_name name_null_;

		lexeme_point end_mismatch_suspect_;

		parse_function_result_stack_type function_stack_;

		utils::hash_map<ast_name, ast_local*> local_map_;
		ast_local_stack_type local_stack_;

		parse_result::parse_errors_type parse_errors_;

		std::vector<std::vector<lexeme_point::token_type>::size_type> match_recovery_stop_on_token_;

		ast_statement_stack_type scratch_statements_;
		ast_expression_stack_type scratch_expressions_;
		ast_expression_stack_type scratch_expression_utils_;
		ast_name_stack_type scratch_names_;
		ast_name_stack_type scratch_pack_names_;
		parse_name_binding_result_stack_type scratch_bindings_;
		ast_local_stack_type scratch_locals_;
		ast_table_property_stack_type scratch_table_properties_;
		ast_type_stack_type scratch_annotations_;
		ast_type_or_pack_type scratch_type_or_pack_annotations_;
		ast_class_property_stack_type scratch_declared_class_properties_;
		ast_table_item_stack_type scratch_items_;
		ast_argument_name_stack_type scratch_argument_names_;
		ast_optional_argument_name_stack_type scratch_optional_argument_names_;
		std::string scratch_data_;

		/**
		 * @note Internal use only.
		 */
		template<typename R, typename T>
		R put_object_to_allocator(const temporary_stack<T>& data);

		template<typename R, typename T>
		R put_object_to_allocator(T data);

		parser(ast_name buffer, ast_name_table& name_table, utils::trivial_allocator& allocator);

		ast_statement_block* parse_chunk();

		// chunk ::= {statement [`;']} [last_statement [`;']]
		// block ::= chunk
		ast_statement_block* parse_block();

		ast_statement_block* parse_block_no_scope();

		// statement ::=
		// var_list `=' expression_list |
		// function_call |
		// do block end |
		// while exp do block end |
		// repeat block until exp |
		// if exp then block {elseif exp then block} [else block] end |
		// for Name `=' exp `,' exp [`,' exp] do block end |
		// for name_list in expression_list do block end |
		// function function_name function_body |
		// local function function_ame function_body |
		// local name_list [`=' expression_list]
		// last_statement ::= return [expression_list] | break
		ast_statement* parse_statement();

		// if exp then block {elseif exp then block} [else block] end
		ast_statement* parse_if();

		// while exp do block end
		ast_statement* parse_while();

		// repeat block until exp
		ast_statement* parse_repeat();

		// do block end
		ast_statement* parse_do();

		// break
		ast_statement* parse_break();

		// continue
		ast_statement* parse_continue(utils::location begin);

		// for name `=' expression `,' expression [`,' expression] do block end |
		// for name_list in expression_list do block end |
		ast_statement* parse_for();

		// function function_name function_body |
		// function_name ::= name {`.' name} [`@' name]
		ast_statement* parse_function_statement();

		// local function function_ame function_body |
		// local name_list [`=' expression_list]
		ast_statement* parse_local();

		// return [expression_list]
		ast_statement* parse_return();

		// `using` alias_name [`<' var_list `>'] `=' type_annotation
		ast_statement* parse_type_alias(utils::location begin, bool exported);

		ast_statement_declare_class::ast_declared_class_property parse_declared_class_method();

		// `declare global' name: type_annotation |
		// `declare function' name`(' [parameter_list] `)' [`:` type_annotation]
		ast_statement* parse_declaration(utils::location begin);

		// var_list `=' expression_list
		ast_statement* parse_assignment(ast_expression* initial);

		// var [`+=' | `-=' | `*=' | `/=' | `%=' | `**=' | `='] expression
		ast_statement* parse_compound_assignment(ast_expression* initial, ast_expression_binary::operand_type operand);

		// function_body ::= `(' [parameter_list] `)' block end
		// parameter_list ::= name_list [`,' `...'] | `...'
		std::pair<ast_expression_function*, ast_local*> parse_function_body(bool has_self, const lexeme_point& match_function, ast_name debug_name, std::optional<parse_name_result> local_name);

		// expression_list ::= {expression `,'} expression
		void parse_expression_list(temporary_stack<ast_expression*>& result);

		// binding ::= name [`:` type_annotation]
		parse_name_binding_result parse_binding();

		// binding_list ::= (binding | `...') {`,' binding_list}
		// returns the location of the vararg ..., or std::nullopt if the function is not vararg.
		std::pair<std::optional<utils::location>, ast_type_pack*> parse_binding_list(temporary_stack<parse_name_binding_result>& result, bool allow_ellipsis = false);

		ast_type* parse_optional_type_annotation();

		// type_list ::= type_annotation [`,' type_list]
		// return_type ::= type_annotation | `(' type_list `)'
		// ast_type_table::ast_table_property ::= name `:' type_annotation
		// ast_type_table::ast_table_indexer ::= `[' type_annotation `]' `:' type_annotation
		// property_list ::= (ast_type_table::ast_table_property | ast_type_table::ast_table_indexer) [`,' property_list]
		// type_annotation
		//      ::= name
		//      |   `null`
		//      |   `{' [property_list] `}'
		//      |   `(' [type_list] `)' `->` return_type
		// returns the variadic annotation, if it exists.
		ast_type_pack* parse_type_list(temporary_stack<ast_type*>& result, temporary_stack<std::optional<ast_argument_name>>& result_names);

		std::optional<ast_type_list> parse_optional_return_type_annotation();
		std::pair<utils::location, ast_type_list> parse_return_type_annotation();

		ast_type_table::ast_table_indexer* parse_table_indexer_annotation();

		ast_type_or_pack parse_function_type_annotation(bool allow_pack);
		ast_type* parse_function_type_annotation_tail(const lexeme_point& begin, generic_names_type generics, generic_names_type generic_packs, ast_array<ast_type*>& params, ast_type_function::argument_names_type& param_names, ast_type_pack* vararg_annotation);

		ast_type* parse_table_type_annotation();
		ast_type_or_pack parse_simple_type_annotation(bool allow_pack);

		ast_type_or_pack parse_type_or_pack_annotation();
		ast_type* parse_type_annotation(temporary_stack<ast_type*>& parts, utils::location begin);
		ast_type* parse_type_annotation();

		ast_type_pack* parse_type_pack_annotation();
		ast_type_pack* parse_variadic_argument_annotation();

		// sub-expression -> (assertion_expression | unary_operand sub-expression) { binary_operand sub-expression }
		// where `binary_operand' is any binary operator with a priority higher than `limit'
		ast_expression* parse_expression(ast_expression_binary::operand_priority_type limit = 0);

		// name
		ast_expression* parse_name_expression(ast_name context = "");

		// prefix_expression -> name | '(' expression ')'
		ast_expression* parse_prefix_expression();

		// primary_expression -> prefix_expression { `.' name | `[' expression `]' | `:' name function_args | function_args }
		ast_expression* parse_primary_expression(bool as_statement);

		// assertion_expression -> simple_expression [`::' type_annotation]
		ast_expression* parse_assertion_expression();

		// simple_expression -> NUMBER | STRING | null | true | false | ... | constructor | FUNCTION body | primary_expression
		ast_expression* parse_simple_expression();

		// args ::=  `(' [expression_list] `)' | table_constructor | String
		ast_expression* parse_function_arguments(ast_expression* function, bool has_self, utils::location self_loc);

		// table_constructor ::= `{' [field_list] `}'
		// field_list ::= field {field_separator field} [field_separator]
		// field ::= `[' expression `]' `=' expression | name `=' expression | expression
		// field_separator ::= `,' | `;'
		ast_expression* parse_table_constructor();

		ast_expression* parse_if_else_expression();

		// name
		std::optional<parse_name_result> parse_name_optional(const char* context = nullptr);
		parse_name_result parse_name(const char* context = nullptr);
		parse_name_result parse_index_name(ast_name context, utils::position previous);

		// `<' name_list `>'
		std::pair<generic_names_type, generic_names_type> parse_generic_type_list();

		// `<' type_annotation [, ...] `>'
		ast_array<ast_type_or_pack> parse_type_params();

		std::optional<ast_array<char>> parse_char_array();
		ast_expression* parse_string();

		ast_local* push_local(const parse_name_binding_result& binding);

		locals_stack_size_type save_locals();

		void restore_locals(locals_stack_size_type offset);

		template<bool Increase, typename... Args>
			requires(std::is_convertible_v<Args, lexeme_point::token_underlying_type> && ...) && ((not std::is_convertible_v<Args, lexeme_point::token_type>) && ...)
		constexpr void count_match_recovery_stop_on_token(Args ... args) noexcept
		{
			([&](const auto arg)
				{
					if constexpr (Increase) { ++match_recovery_stop_on_token_[arg]; }
					else { --match_recovery_stop_on_token_[arg]; }
				}(args),
				...);
		}

		template<bool Increase, typename... Args>
			requires(std::is_convertible_v<Args, lexeme_point::token_type> && ...)
		constexpr void count_match_recovery_stop_on_token(Args ... args) noexcept { count_match_recovery_stop_on_token<Increase>(static_cast<lexeme_point::token_underlying_type>(static_cast<lexeme_point::token_type>(args))...); }

		// check that parser is at lexeme_point/symbol, move to next lexeme_point/symbol on success, report failure and continue on failure
		bool expect_and_consume(lexeme_point::token_underlying_type type, const char* context = nullptr);
		bool expect_and_consume(lexeme_point::token_type type, const char* context = nullptr);
		void expect_and_consume_fail(lexeme_point::token_type type, const char* context);

		bool expect_match_and_consume(lexeme_point::token_underlying_type type, const lexeme_point& begin, bool search_for_missing = false);
		bool expect_match_and_consume(lexeme_point::token_type type, const lexeme_point& begin, bool search_for_missing = false);
		void expect_match_and_consume_fail(lexeme_point::token_type type, const lexeme_point& begin, const char* extra = nullptr);

		bool expect_match_end_and_consume(lexeme_point::token_type type, const lexeme_point& begin);
		void expect_match_end_and_consume_fail(lexeme_point::token_type type, const lexeme_point& begin);

		void increase_recursion_counter(const char* context);

		void report(utils::location loc, std::string message);

		void report_name_error(const char* context);

		ast_statement_error* report_statement_error(utils::location loc, ast_statement_error::error_expressions_type expressions, ast_statement_error::error_statements_type statements, std::string message);

		ast_expression_error* report_expression_error(utils::location loc, ast_expression_error::error_expressions_type expressions, std::string message);

		ast_type_error* report_type_annotation_error(utils::location, ast_type_error::error_types_type types, bool is_missing, std::string message);

		const lexeme_point& next_lexeme_point();

	public:
		// for name_self
		constexpr static lexeme_point::keyword_literal_type keyword_self{"self"};
		// for name_number
		constexpr static lexeme_point::keyword_literal_type keyword_number{"number"};
		// for name_error
		constexpr static lexeme_point::keyword_literal_type keyword_error{"%error-id%"};
		// for name_null
		constexpr static lexeme_point::keyword_literal_type keyword_null{"null"};

		static parse_result parse(ast_name buffer, ast_name_table& name_table, parse_options options = {});
	};
}

#endif // GAL_LANG_AST_PARSER_HPP
