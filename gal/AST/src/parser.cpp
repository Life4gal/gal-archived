#include <ast/parser.hpp>

#include <ast/lexer.hpp>

namespace gal::ast
{
	template<typename R, typename T>
	R parser::put_object_to_allocator(const ast_stack<T>& data)
	{
		// todo
		// auto result = R{static_cast<T>(allocator_.allocate(sizeof(T) * data.size())), data.size()};
		// std::uninitialized_copy(data.begin(), data.end(), result.data());
		// return result;

		(void)data;
		return R{};
	}

	template<typename R, typename T>
	R parser::put_object_to_allocator(T data)
	{
		// todo
		// auto result = R{static_cast<T>(allocator_.allocate(sizeof(T) * 1)), 1};
		// std::construct_at(result.data(), data);
		// return result;

		(void)data;
		return R{};
	}

	parser::parser(const ast_name buffer, ast_name_table& name_table, utils::trivial_allocator& allocator)
		: lexer_{buffer, name_table},
		  allocator_{allocator},
		  recursion_counter_{0},
		  name_self_{name_table.insert(keyword_self)},
		  name_number_{name_table.insert(keyword_number)},
		  name_error_{name_table.insert(keyword_error)},
		  name_null_{name_table.insert(keyword_null)},
		  end_mismatch_suspect_{lexeme_point::bad_lexeme_point(utils::location{})},
		  match_recovery_stop_on_token_(static_cast<decltype(match_recovery_stop_on_token_)::size_type>(lexeme_point::token_type::keyword_sentinel_end), 0)
	{
		function_stack_.emplace(true, 0);

		match_recovery_stop_on_token_[lexeme_point::token_to_scalar(lexeme_point::token_type::eof)] = 1;

		// read first lexeme point
		next_lexeme_point();
	}

	ast_statement_block* parser::parse_chunk()
	{
		auto* result = parse_block();

		if (not lexer_.current().is_end_point()) { expect_and_consume_fail(lexeme_point::token_type::eof, nullptr); }

		return result;
	}

	ast_statement_block* parser::parse_block()
	{
		const auto locals_begin = save_locals();

		auto* result = parse_block_no_scope();

		restore_locals(locals_begin);

		return result;
	}

	ast_statement_block* parser::parse_block_no_scope()
	{
		auto body{scratch_statements_};

		const auto previous_position = lexer_.previous_location().end;

		while (not lexer_.current().has_follower())
		{
			const auto previous_recursion_counter = recursion_counter_;

			increase_recursion_counter("block");

			auto* statement = parse_statement();

			recursion_counter_ = previous_recursion_counter;

			if (lexer_.current().is_any_type_of(lexeme_point::get_semicolon_symbol()))
			{
				next_lexeme_point();
				statement->set_semicolon(true);
			}

			body.push(statement);

			if (not statement->has_statement_follow()) { break; }
		}

		return allocator_.new_object<ast_statement_block>(utils::location{previous_position, lexer_.current().get_location().begin}, put_object_to_allocator<ast_statement_block::block_body_type>(body));
	}

	ast_statement* parser::parse_statement()
	{
		using enum lexeme_point::token_type;
		switch (lexer_.current().get_type())// NOLINT(clang-diagnostic-switch-enum)
		{
			case keyword_break: { return parse_break(); }
			case keyword_do: { return parse_do(); }
			case keyword_for: { return parse_for(); }
			case keyword_function: { return parse_function_statement(); }
			case keyword_if: { return parse_if(); }
			case keyword_local: { return parse_local(); }
			case keyword_repeat: { return parse_repeat(); }
			case keyword_return: { return parse_return(); }
			case keyword_while: { return parse_while(); }
			default: { break; }
		}

		const auto begin = lexer_.current().get_location();

		// we need to disambiguate a few cases, primarily assignment (lvalue = ...) vs statements-that-are calls
		auto* expression = parse_primary_expression(true);

		if (expression->is<ast_expression_call>()) { return allocator_.new_object<ast_statement_expression>(expression->get_location(), expression); }

		// if the next token is , or =, it's an assignment (, means it's an assignment with multiple variables)
		if (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol(), lexeme_point::get_assignment_symbol())) { return parse_assignment(expression); }

		// if the next token is a compound assignment operator, it's a compound assignment (these don't support multiple variables)
		if (const auto operand = lexer_.current().to_compound_operand(); operand.has_value()) { return parse_compound_assignment(expression, operand.value()); }

		// we know this isn't a call or an assignment; therefore it must be a context-sensitive keyword such as `using` or `continue`
		const auto identifier = expression->get_identifier();

		if (options_.allow_type_annotations)
		{
			if (identifier == lexeme_point::get_type_alias_keyword()) { return parse_type_alias(expression->get_location(), false); }
			if (identifier == lexeme_point::get_export_keyword() && lexer_.current().is_any_type_of(name) && lexer_.current().get_data_or_name() == lexeme_point::get_type_alias_keyword())
			{
				next_lexeme_point();
				return parse_type_alias(expression->get_location(), true);
			}
		}

		if (options_.support_continue_statement && identifier == lexeme_point::get_continue_keyword()) { return parse_continue(expression->get_location()); }

		if (options_.allow_type_annotations && options_.allow_declaration_syntax && identifier == lexeme_point::get_declare_keyword()) { return parse_declaration(expression->get_location()); }

		// skip unexpected symbol if lexer couldn't advance at all (statements are parsed in a loop)
		if (begin == lexer_.current().get_location()) { next_lexeme_point(); }

		return report_statement_error(expression->get_location(), put_object_to_allocator<ast_statement_error::error_expressions_type>(expression), {}, "Incomplete statement: expected assignment or a function call.");
	}

	ast_statement* parser::parse_if()
	{
		const auto begin = lexer_.current().get_location();

		next_lexeme_point();// if / elif

		auto* condition = parse_expression();

		const auto match_then = lexer_.current();
		const bool has_then = expect_and_consume(lexeme_point::token_type::keyword_then, "if statement");

		auto* then_body = parse_block();

		std::optional<utils::location> else_location;
		ast_statement* else_body = nullptr;
		decltype(lexer_.current().get_location()) end;
		bool has_end;

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_elif))
		{
			else_location = lexer_.current().get_location();
			else_body = parse_if();
			end = else_body->get_location();
			has_end = else_body->as<ast_statement_if>()->has_end();
		}
		else
		{
			auto match_then_else = match_then;

			if (lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_else))
			{
				else_location = lexer_.current().get_location();
				match_then_else = lexer_.current();
				next_lexeme_point();

				else_body = parse_block();
				else_body->reset_location_begin(match_then_else.get_location().end);
			}

			end = lexer_.current().get_location();
			has_end = expect_match_and_consume(lexeme_point::token_type::keyword_end, match_then_else);
		}

		return allocator_.new_object<ast_statement_if>(make_longest_line(begin, end), condition, then_body, else_body, has_then ? std::make_optional(match_then.get_location()) : std::nullopt, else_location, has_end);
	}

	ast_statement* parser::parse_while()
	{
		const auto begin = lexer_.current().get_location();

		next_lexeme_point();// while

		auto* condition = parse_expression();

		const auto match_do = lexer_.current();
		const bool has_do = expect_and_consume(lexeme_point::token_type::keyword_do, "while loop");

		++function_stack_.top().loop_depth;

		auto* body = parse_block();

		--function_stack_.top().loop_depth;

		const auto end = lexer_.current().get_location();

		bool has_end = expect_match_end_and_consume(lexeme_point::token_type::keyword_end, match_do);

		return allocator_.new_object<ast_statement_while>(make_longest_line(begin, end), condition, body, has_do ? std::make_optional(match_do.get_location()) : std::nullopt, has_end);
	}

	ast_statement* parser::parse_repeat()
	{
		const auto begin = lexer_.current().get_location();

		const auto match_repeat = lexer_.current();
		next_lexeme_point();// repeat

		const auto locals_begin = save_locals();

		++function_stack_.top().loop_depth;

		auto* body = parse_block_no_scope();

		--function_stack_.top().loop_depth;

		const auto has_until = expect_match_end_and_consume(lexeme_point::token_type::keyword_until, match_repeat);

		auto* condition = parse_expression();

		restore_locals(locals_begin);

		return allocator_.new_object<ast_statement_repeat>(make_longest_line(begin, condition->get_location()), condition, body, has_until);
	}

	ast_statement* parser::parse_do()
	{
		const auto [begin, _] = lexer_.current().get_location();

		const auto match_do = lexer_.current();
		next_lexeme_point();// do

		auto* body = parse_block();

		body->reset_location_begin(begin);

		expect_match_end_and_consume(lexeme_point::token_type::keyword_end, match_do);

		return body;
	}


	ast_statement* parser::parse_break()
	{
		const auto begin = lexer_.current().get_location();
		next_lexeme_point();// break

		auto* ret = allocator_.new_object<ast_statement_continue>(begin);

		if (function_stack_.top().is_root()) { return report_statement_error(begin, {}, put_object_to_allocator<ast_statement_error::error_statements_type>(ret), "break statement must be inside a loop."); }

		return ret;
	}

	ast_statement* parser::parse_continue(utils::location begin)
	{
		auto* ret = allocator_.new_object<ast_statement_continue>(begin);

		if (function_stack_.top().is_root()) { return report_statement_error(begin, {}, put_object_to_allocator<ast_statement_error::error_statements_type>(ret), "continue statement must be inside a loop."); }

		return ret;
	}
}
