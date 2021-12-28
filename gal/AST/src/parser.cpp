#include <ast/parser.hpp>

#include <ast/lexer.hpp>

namespace gal::ast
{
	template<typename R, typename T>
	R parser::put_object_to_allocator(const temporary_stack<T>& data)
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
		function_stack_.emplace_back(true, 0);

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
		temporary_stack body{scratch_statements_};

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

		++function_stack_.back().loop_depth;

		auto* body = parse_block();

		--function_stack_.back().loop_depth;

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

		++function_stack_.back().loop_depth;

		auto* body = parse_block_no_scope();

		--function_stack_.back().loop_depth;

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

		if (function_stack_.back().is_root()) { return report_statement_error(begin, {}, put_object_to_allocator<ast_statement_error::error_statements_type>(ret), "break statement must be inside a loop."); }

		return ret;
	}

	ast_statement* parser::parse_continue(utils::location begin)
	{
		auto* ret = allocator_.new_object<ast_statement_continue>(begin);

		if (function_stack_.back().is_root()) { return report_statement_error(begin, {}, put_object_to_allocator<ast_statement_error::error_statements_type>(ret), "continue statement must be inside a loop."); }

		return ret;
	}

	ast_statement* parser::parse_for()
	{
		const auto begin = lexer_.current().get_location();

		next_lexeme_point();// for

		const auto var_name = parse_binding();

		if (lexer_.current().is_any_type_of(lexeme_point::get_assignment_symbol()))
		{
			next_lexeme_point();// =

			auto* from = parse_expression();

			expect_and_consume(lexeme_point::get_comma_symbol(), "index range");

			auto* to = parse_expression();

			decltype(to) step = nullptr;

			if (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol()))
			{
				next_lexeme_point();// ,
				step = parse_expression();
			}

			const auto match_do = lexer_.current();
			const bool has_do = expect_and_consume(lexeme_point::token_type::keyword_do, "for loop");

			const auto locals_begin = save_locals();

			++function_stack_.back().loop_depth;

			auto* var = push_local(var_name);

			auto* body = parse_block();

			--function_stack_.back().loop_depth;

			restore_locals(locals_begin);

			const auto end = lexer_.current().get_location();
			bool has_end = expect_match_end_and_consume(lexeme_point::token_type::keyword_end, match_do);

			return allocator_.new_object<ast_statement_for>(
					make_longest_line(begin, end),
					var,
					from,
					to,
					step,
					body,
					has_do ? std::make_optional(match_do.get_location()) : std::nullopt,
					has_end
					);
		}

		temporary_stack names{scratch_bindings_};
		names.push(var_name);

		if (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol()))
		{
			next_lexeme_point();// ,

			parse_binding_list(names);
		}

		const auto in_loc = lexer_.current().get_location();
		const bool has_in = expect_and_consume(lexeme_point::token_type::keyword_in, "for loop");

		temporary_stack values{scratch_expressions_};
		parse_expression_list(values);

		const auto match_do = lexer_.current();
		const bool has_do = expect_and_consume(lexeme_point::token_type::keyword_do, "for loop");

		const auto locals_begin = save_locals();

		++function_stack_.back().loop_depth;

		temporary_stack vars{scratch_locals_};
		vars.insert(names, [this](auto&& top) -> decltype(auto) { return push_local(top); });

		auto* body = parse_block();

		--function_stack_.back().loop_depth;

		restore_locals(locals_begin);

		const auto end = lexer_.current().get_location();
		bool has_end = expect_match_end_and_consume(lexeme_point::token_type::keyword_end, match_do);

		return allocator_.new_object<ast_statement_for_in>(
				make_longest_line(begin, end),
				put_object_to_allocator<ast_statement_for_in::var_locals_type>(vars),
				put_object_to_allocator<ast_statement_for_in::value_expressions_type>(values),
				body,
				has_in ? std::make_optional(in_loc) : std::nullopt,
				has_do ? std::make_optional(match_do.get_location()) : std::nullopt,
				has_end);
	}

	ast_statement* parser::parse_function_statement()
	{
		const auto begin = lexer_.current().get_location();

		const auto match_function = lexer_.current();

		next_lexeme_point();// function

		auto debug_name = lexer_.current().is_any_type_of(lexeme_point::token_type::name) ? lexer_.current().get_data_or_name() : decltype(lexer_.current().get_data_or_name()){};

		// parse function_name into a chain of indexing operators
		auto* expression = parse_name_expression("function name");

		const auto previous_recursion_counter = recursion_counter_;

		while (lexer_.current().is_any_type_of(lexeme_point::get_dot_symbol()))
		{
			const auto operand_pos = lexer_.current().get_location().begin;
			next_lexeme_point();// .

			auto [name, loc] = parse_name("field name");

			// while we could concatenate the name chain, for now let's just write the short name
			debug_name = name;

			expression = allocator_.new_object<ast_expression_index_name>(
					make_longest_line(begin, loc),
					expression,
					name,
					loc,
					operand_pos,
					lexeme_point::get_dot_symbol());

			// note: while the parser isn't recursive here, we're generating recursive structures of unbounded depth
			increase_recursion_counter("function name");
		}

		recursion_counter_ = previous_recursion_counter;

		// finish with @
		// function x.y.z@foo(parameter_list) body end ==> x.y.z.foo = function(self, parameter_list) body end
		bool has_self = false;
		if (lexer_.current().is_any_type_of(lexeme_point::get_at_symbol()))
		{
			const auto operand_pos = lexer_.current().get_location().begin;

			next_lexeme_point();// @

			auto [name, loc] = parse_name("method name");

			// while we could concatenate the name chain, for now let's just write the short name
			debug_name = name;

			expression = allocator_.new_object<ast_expression_index_name>(
					make_longest_line(begin, loc),
					expression,
					name,
					loc,
					operand_pos,
					lexeme_point::get_at_symbol());

			has_self = true;
		}

		count_match_recovery_stop_on_token<true>(lexeme_point::token_type::keyword_end);

		auto* body = parse_function_body(has_self, match_function, debug_name, std::nullopt).first;

		count_match_recovery_stop_on_token<false>(lexeme_point::token_type::keyword_end);

		return allocator_.new_object<ast_statement_function>(
				make_longest_line(begin, body->get_location()),
				expression,
				body);
	}

	ast_statement* parser::parse_local()
	{
		const auto begin = lexer_.current().get_location();

		next_lexeme_point();// local

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_function))
		{
			auto match_function = lexer_.current();

			next_lexeme_point();// function

			// match_function is only used for diagnostics; to make it suitable for detecting missed indentation between
			// `local function` and `end`, we patch the token to begin at the column where `local` starts
			if (const auto [func_begin, func_end] = match_function.get_location(); func_begin.line == begin.begin.line) { match_function.reset_location({{func_begin.line, begin.begin.column}, func_end}); }

			auto name = parse_name("variable name");

			count_match_recovery_stop_on_token<true>(lexeme_point::token_type::keyword_end);

			auto [body, var] = parse_function_body(false, match_function, name.name, name);

			count_match_recovery_stop_on_token<false>(lexeme_point::token_type::keyword_end);

			return allocator_.new_object<ast_statement_function_local>(
					utils::location{begin.begin, body->get_location().end},
					var,
					body);
		}

		count_match_recovery_stop_on_token<true>(lexeme_point::get_assignment_symbol());

		temporary_stack names{scratch_bindings_};
		parse_binding_list(names);

		count_match_recovery_stop_on_token<false>(lexeme_point::get_assignment_symbol());

		temporary_stack vars{scratch_locals_};

		temporary_stack values{scratch_expressions_};

		std::optional<utils::location> assignment_loc;

		if (lexer_.current().is_any_type_of(lexeme_point::get_assignment_symbol()))
		{
			assignment_loc = lexer_.current().get_location();

			next_lexeme_point();// =

			parse_expression_list(values);
		}

		vars.insert(names, [this](auto&& top) -> decltype(auto) { return push_local(top); });

		const auto end = values.empty() ? lexer_.previous_location() : values.top()->get_location();

		return allocator_.new_object<ast_statement_local>(
				make_longest_line(begin, end),
				put_object_to_allocator<ast_statement_local::var_locals_type>(vars),
				put_object_to_allocator<ast_statement_local::value_expressions_type>(values),
				assignment_loc);
	}

	ast_statement* parser::parse_return()
	{
		const auto begin = lexer_.current().get_location();

		next_lexeme_point();// return

		temporary_stack list{scratch_expressions_};

		if (not lexer_.current().has_follower() && not lexer_.current().is_any_type_of(lexeme_point::get_semicolon_symbol())) { parse_expression_list(list); }

		const auto end = list.empty() ? begin : list.top()->get_location();

		return allocator_.new_object<ast_statement_return>(
				make_longest_line(begin, end),
				put_object_to_allocator<ast_statement_return::expression_list_type>(list));
	}

	ast_statement* parser::parse_type_alias(utils::location begin, bool exported)
	{
		// note: `using` token is already parsed for us, so we just need to parse the rest

		auto name = parse_name_optional("using name");

		// Use error name if the name is missing
		if (not name.has_value()) { name = {name_error_, lexer_.current().get_location()}; }

		auto [generics, generic_packs] = parse_generic_type_list();

		expect_and_consume(lexeme_point::get_assignment_symbol(), "type alias");

		auto* type = parse_type_annotation();

		return allocator_.new_object<ast_statement_type_alias>(
				make_longest_line(begin, type->get_location()),
				name->name,
				generics,
				generic_packs,
				type,
				exported);
	}

	ast_statement_declare_class::ast_declared_class_property parser::parse_declared_class_method()
	{
		next_lexeme_point();

		const auto begin = lexer_.current().get_location();

		const auto function_name = parse_name("function name");

		generic_names_type generics;
		generic_names_type generic_packs;

		const auto match_paren = lexer_.current();
		expect_and_consume(lexeme_point::get_parentheses_bracket_open_symbol(), "function parameter list begin");

		temporary_stack args{scratch_bindings_};

		std::optional<utils::location> vararg;
		ast_type_pack* vararg_annotation = nullptr;

		if (not lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_close_symbol())) { std::tie(vararg, vararg_annotation) = parse_binding_list(args, true); }

		expect_match_and_consume(lexeme_point::get_parentheses_bracket_close_symbol(), match_paren);

		auto return_type = parse_optional_return_type_annotation().value_or(ast_type_list{});

		const auto end = lexer_.current().get_location();

		temporary_stack vars{scratch_annotations_};
		temporary_stack var_names{scratch_optional_argument_names_};

		if (args.empty() ||
		    args.bottom().name.name != lexeme_point::get_self_keyword() ||
		    args.bottom().annotation
		)
		{
			return {
					function_name.name,
					report_type_annotation_error(make_longest_line(begin, end), {}, false, std_format::format("'{}' must be present as the unannotated first parameter", lexeme_point::get_self_keyword())),
					true};
		}

		// Skip the first index.
		for (decltype(args.size()) i = 1; i < args.size(); ++i)
		{
			const auto& [name, annotation] = args[i];

			var_names.emplace(ast_argument_name{name.name, name.loc});

			if (annotation) { vars.push(annotation); }
			else { vars.push(report_type_annotation_error(make_longest_line(begin, end), {}, false, std_format::format("No.{} declaration parameters aside from '{}' should but no be annotated", i, lexeme_point::get_self_keyword()))); }
		}

		if (vararg.has_value() && not vararg_annotation) { report(begin, std_format::format("All declaration parameters aside from '{}' must be annotated", lexeme_point::get_self_keyword())); }

		auto* function_type = allocator_.new_object<ast_type_function>(
				make_longest_line(begin, end),
				generics,
				generic_packs,
				ast_type_list{put_object_to_allocator<ast_array<ast_type*>>(vars), vararg_annotation},
				put_object_to_allocator<ast_type_function::argument_names_type>(var_names),
				return_type);

		return {function_name.name, function_type, true};
	}

	ast_statement* parser::parse_declaration(utils::location begin)
	{
		// `declare` token is already parsed at this point
		if (lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_function))
		{
			next_lexeme_point();// function

			const auto global_name = parse_name("global function name");

			auto [generics, generic_packs] = parse_generic_type_list();

			const auto match_paren = lexer_.current();

			expect_and_consume(lexeme_point::get_parentheses_bracket_open_symbol(), "global function declaration");

			temporary_stack args{scratch_bindings_};

			std::optional<utils::location> vararg;
			ast_type_pack* vararg_annotation = nullptr;

			if (not lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_close_symbol())) { std::tie(vararg, vararg_annotation) = parse_binding_list(args, true); }

			expect_match_and_consume(lexeme_point::get_parentheses_bracket_close_symbol(), match_paren);

			auto return_types = parse_optional_return_type_annotation().value_or(ast_type_list{});

			const auto end = lexer_.current().get_location();

			temporary_stack vars{scratch_annotations_};
			temporary_stack var_names{scratch_argument_names_};

			for (decltype(args.size()) i = 0; i < args.size(); ++i)
			{
				const auto& [name, annotation] = args[i];

				if (not annotation) { return report_statement_error(make_longest_line(begin, end), {}, {}, std_format::format("No.{} declaration parameters should but not be annotated", i)); }

				vars.push(annotation);
				var_names.emplace(name.name, name.loc);
			}

			if (vararg.has_value() && not vararg_annotation) { return report_statement_error(make_longest_line(begin, end), {}, {}, "All declaration parameters must be annotated"); }

			return allocator_.new_object<ast_statement_declare_function>(
					make_longest_line(begin, end),
					global_name.name,
					generics,
					generic_packs,
					ast_type_list{put_object_to_allocator<ast_array<ast_type*>>(vars), vararg_annotation},
					put_object_to_allocator<ast_statement_declare_function::arguments_type>(var_names),
					return_types);
		}
		if (lexer_.current().is_any_type_of(lexeme_point::token_type::name) && lexer_.current().get_data_or_name() == lexeme_point::get_class_keyword())
		{
			next_lexeme_point();// class

			const auto class_begin = lexer_.current().get_location();

			const auto class_name = parse_name("class name");

			std::optional<ast_name> super_name;

			if (lexer_.current().is_any_type_of(lexeme_point::token_type::name) && lexer_.current().get_data_or_name() == lexeme_point::get_extend_keyword())
			{
				next_lexeme_point();// extends

				super_name = parse_name("superclass name").name;
			}

			temporary_stack properties{scratch_declared_class_properties_};

			// There are two possibilities: Either it's a property or a function.
			while (not lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_end))
			{
				if (lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_function)) { properties.push(parse_declared_class_method()); }
				else
				{
					const auto property_name = parse_name("property name");
					expect_and_consume(lexeme_point::get_colon_symbol(), "property type annotation");
					auto* property_type = parse_type_annotation();
					properties.emplace(property_name.name, property_type, false);
				}
			}

			const auto class_end = lexer_.current().get_location();

			next_lexeme_point();// end

			return allocator_.new_object<ast_statement_declare_class>(
					make_longest_line(class_begin, class_end),
					class_name.name,
					super_name,
					put_object_to_allocator<ast_statement_declare_class::class_properties_type>(properties));
		}
		if (const auto global_name = parse_name_optional("global variable name"); global_name.has_value())
		{
			expect_and_consume(lexeme_point::get_colon_symbol(), "global variable declaration");

			auto* type = parse_type_annotation();
			return allocator_.new_object<ast_statement_declare_global>(
					make_longest_line(begin, type->get_location()),
					global_name->name,
					type);
		}
		return report_statement_error(begin, {}, {}, std_format::format("declare must be followed by an identifier, 'function', or '{}'", lexeme_point::get_class_keyword()));
	}

	ast_statement* parser::parse_assignment(ast_expression* initial)
	{
		if (not initial->is_lvalue()) { initial = report_expression_error(initial->get_location(), put_object_to_allocator<ast_expression_error::error_expressions_type>(initial), "Assigned expression must be a variable or a field"); }

		temporary_stack vars{scratch_expressions_};
		vars.push(initial);

		while (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol()))
		{
			next_lexeme_point();// ,

			auto* expression = parse_primary_expression(false);

			if (not expression->is_lvalue()) { expression = report_expression_error(expression->get_location(), put_object_to_allocator<ast_expression_error::error_expressions_type>(expression), "Assigned expression must be a variable or a field"); }

			vars.push(expression);
		}

		expect_and_consume(lexeme_point::get_assignment_symbol(), "assignment");

		temporary_stack values{scratch_expression_utils_};
		parse_expression_list(values);

		return allocator_.new_object<ast_statement_assign>(
				make_longest_line(initial->get_location(), values.top()->get_location()),
				put_object_to_allocator<ast_statement_assign::var_expressions_type>(vars),
				put_object_to_allocator<ast_statement_assign::value_expressions_type>(values));
	}

	ast_statement* parser::parse_compound_assignment(ast_expression* initial, ast_expression_binary::operand_type operand)
	{
		if (not initial->is_lvalue()) { initial = report_expression_error(initial->get_location(), put_object_to_allocator<ast_expression_error::error_expressions_type>(initial), "Assigned expression must be a variable or a field"); }

		next_lexeme_point();// xxx=

		auto* value = parse_expression();

		return allocator_.new_object<ast_statement_compound_assign>(
				make_longest_line(initial->get_location(), value->get_location()),
				operand,
				initial,
				value);
	}

	std::pair<ast_expression_function*, ast_local*> parser::parse_function_body(bool has_self, const lexeme_point& match_function, ast_name debug_name, std::optional<parse_name_result> local_name)
	{
		const auto begin = match_function.get_location();

		auto [generics, generic_packs] = parse_generic_type_list();

		const auto match_paren = lexer_.current();

		expect_and_consume(lexeme_point::get_parentheses_bracket_open_symbol(), "function");

		temporary_stack args{scratch_bindings_};

		std::optional<utils::location> vararg;
		ast_type_pack* vararg_annotation = nullptr;

		if (not lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_close_symbol())) { std::tie(vararg, vararg_annotation) = parse_binding_list(args, true); }

		const auto arg_loc = match_paren.is_any_type_of(lexeme_point::get_parentheses_bracket_open_symbol()) && lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_close_symbol())
			                     ? std::make_optional(utils::location{match_paren.get_location().begin, lexer_.current().get_location().end})
			                     : std::nullopt;

		expect_match_and_consume(lexeme_point::get_parentheses_bracket_close_symbol(), match_paren, true);

		auto type_list = parse_optional_return_type_annotation();

		auto* function_local = local_name.has_value() ? push_local({local_name.value(), nullptr}) : nullptr;

		const auto locals_begin = save_locals();

		function_stack_.emplace_back(vararg.has_value(), 0);

		auto* self = has_self ? push_local({{name_self_, begin}, nullptr}) : nullptr;

		temporary_stack vars{scratch_locals_};

		vars.insert(args, [this](auto&& top) -> decltype(auto) { return push_local(top); });

		auto* body = parse_block();

		function_stack_.pop_back();

		restore_locals(locals_begin);

		const auto end = lexer_.current().get_location();

		auto has_end = expect_match_end_and_consume(lexeme_point::token_type::keyword_end, match_function);

		return std::make_pair(
				allocator_.new_object<ast_expression_function>(
						make_longest_line(begin, end),
						generics,
						generic_packs,
						self,
						put_object_to_allocator<ast_expression_function::args_locals_type>(vars),
						vararg,
						body,
						function_stack_.size(),
						debug_name,
						type_list,
						vararg_annotation,
						has_end,
						arg_loc
						),
				function_local
				);
	}

	void parser::parse_expression_list(temporary_stack<ast_expression*>& result)
	{
		result.push(parse_expression());

		while (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol()))
		{
			next_lexeme_point();// ,

			result.push(parse_expression());
		}
	}

	parser::parse_name_binding_result parser::parse_binding()
	{
		auto name = parse_name_optional("variable name");

		// Use placeholder if the name is missing
		if (not name.has_value()) { name = {name_error_, lexer_.current().get_location()}; }

		auto* annotation = parse_optional_type_annotation();

		return {name.value(), annotation};
	}

	std::pair<std::optional<utils::location>, ast_type_pack*> parser::parse_binding_list(temporary_stack<parse_name_binding_result>& result, bool allow_ellipsis)
	{
		while (true)
		{
			if (lexer_.current().is_any_type_of(lexeme_point::token_type::ellipsis) && allow_ellipsis)
			{
				const auto vararg_loc = lexer_.current().get_location();

				next_lexeme_point();// ...

				ast_type_pack* tail_annotation = nullptr;
				if (lexer_.current().is_any_type_of(lexeme_point::get_colon_symbol()))
				{
					next_lexeme_point();// :

					tail_annotation = parse_variadic_argument_annotation();
				}

				return {vararg_loc, tail_annotation};
			}

			result.push(parse_binding());

			if (not lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol())) { break; }

			next_lexeme_point();// ,
		}

		return {std::nullopt, nullptr};
	}

	ast_type* parser::parse_optional_type_annotation()
	{
		if (options_.allow_type_annotations && lexer_.current().is_any_type_of(lexeme_point::get_colon_symbol()))
		{
			next_lexeme_point();// :

			return parse_type_annotation();
		}

		return nullptr;
	}

	ast_type_pack* parser::parse_type_list(temporary_stack<ast_type*>& result, temporary_stack<std::optional<ast_argument_name>>& result_names)
	{
		while (true)
		{
			if (lexer_.current().is_any_type_of(lexeme_point::token_type::ellipsis) || (lexer_.current().is_any_type_of(lexeme_point::token_type::name) && lexer_.peek_next().is_any_type_of(lexeme_point::token_type::ellipsis))) { return parse_type_pack_annotation(); }

			if (lexer_.current().is_any_type_of(lexeme_point::token_type::name) && lexer_.peek_next().is_any_type_of(lexeme_point::get_colon_symbol()))
			{
				// Fill in previous argument names with empty slots
				while (result_names.size() < result.size()) { result_names.emplace(); }

				result_names.emplace(ast_argument_name{{lexer_.current().get_data_or_name()}, lexer_.current().get_location()});
				lexer_.next();

				expect_and_consume(lexeme_point::get_colon_symbol());
			}
			else if (not result_names.empty())
			{
				// If we have a type with named arguments, provide elements for all types
				result_names.emplace();
			}

			result.push(parse_type_annotation());
			if (not lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol())) { break; }

			next_lexeme_point();// ,
		}

		return nullptr;
	}

	std::optional<ast_type_list> parser::parse_optional_return_type_annotation()
	{
		if (options_.allow_type_annotations && lexer_.current().is_any_type_of(lexeme_point::get_colon_symbol()))
		{
			next_lexeme_point();// :

			const auto previous_recursion_count = recursion_counter_;

			auto [loc, result] = parse_return_type_annotation();

			// At this point, if we find a `,` character, it indicates that there are multiple return types
			// in this type annotation, but the list wasn't wrapped in parentheses.
			if (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol()))
			{
				report(lexer_.current().get_location(), std_format::format("Expected a statement, got '{}'; did you forget to wrap the list of return types in `{}{}`?", lexeme_point::get_comma_symbol(), lexeme_point::get_parentheses_bracket_open_symbol(), lexeme_point::get_parentheses_bracket_close_symbol()));

				next_lexeme_point();// ,
			}

			recursion_counter_ = previous_recursion_count;

			return result;
		}

		return std::nullopt;
	}

	std::pair<utils::location, ast_type_list> parser::parse_return_type_annotation()
	{
		increase_recursion_counter("type annotation");

		temporary_stack result{scratch_annotations_};
		temporary_stack result_names{scratch_optional_argument_names_};

		ast_type_pack* vararg_annotation = nullptr;

		const auto begin = lexer_.current();

		if (not lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_open_symbol()))
		{
			if (lexer_.current().is_any_type_of(lexeme_point::token_type::ellipsis) || (lexer_.current().is_any_type_of(lexeme_point::token_type::name) && lexer_.peek_next().is_any_type_of(lexeme_point::token_type::ellipsis))) { vararg_annotation = parse_type_pack_annotation(); }
			else { result.push(parse_type_annotation()); }

			const auto result_location = result.empty() ? vararg_annotation->get_location() : result.bottom()->get_location();

			return {result_location, {put_object_to_allocator<ast_type_list::types_list_type>(result), vararg_annotation}};
		}

		next_lexeme_point();// (

		const auto inner_begin = lexer_.current().get_location();

		count_match_recovery_stop_on_token<true>(lexeme_point::token_type::right_arrow);

		// possibly () -> ReturnType
		if (not lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_close_symbol())) { vararg_annotation = parse_type_list(result, result_names); }

		const auto loc = make_longest_line(begin.get_location(), lexer_.current().get_location());

		expect_match_and_consume(lexeme_point::get_parentheses_bracket_close_symbol(), begin, true);

		count_match_recovery_stop_on_token<false>(lexeme_point::token_type::right_arrow);

		if (not lexer_.current().is_any_type_of(lexeme_point::token_type::right_arrow) && result_names.empty())
		{
			// If it turns out that it's just a '()', it's possible that there are unions/intersections to follow, so fold over it.
			if (result.size() == 1)
			{
				auto* return_type = parse_type_annotation(result, inner_begin);

				return
				{
						make_longest_line(loc, return_type->get_location()),
						{put_object_to_allocator<ast_type_list::types_list_type>(return_type), vararg_annotation}
				};
			}

			return {
					loc,
					{put_object_to_allocator<ast_type_list::types_list_type>(result), vararg_annotation}};
		}

		auto types = put_object_to_allocator<ast_type_list::types_list_type>(result);
		auto names = put_object_to_allocator<ast_type_function::argument_names_type>(result_names);

		temporary_stack fallback_return_types{scratch_annotations_};
		fallback_return_types.push(parse_function_type_annotation_tail(begin, {}, {}, types, names, vararg_annotation));

		return {
				make_longest_line(loc, fallback_return_types.bottom()->get_location()),
				{put_object_to_allocator<ast_type_list::types_list_type>(fallback_return_types), vararg_annotation}
		};
	}

	ast_type_table::ast_table_indexer* parser::parse_table_indexer_annotation()
	{
		const auto begin = lexer_.current();

		next_lexeme_point();// [

		auto* index = parse_type_annotation();

		expect_match_and_consume(lexeme_point::get_parentheses_bracket_close_symbol(), begin);

		expect_and_consume(lexeme_point::get_colon_symbol(), "table field");

		auto* result = parse_type_annotation();

		return allocator_.new_object<ast_type_table::ast_table_indexer>(
				index,
				result,
				make_longest_line(begin.get_location(), result->get_location()));
	}

	ast_type_or_pack parser::parse_function_type_annotation(const bool allow_pack)
	{
		increase_recursion_counter("type annotation");

		const auto monomorphic = not lexer_.current().is_any_type_of(lexeme_point::get_less_than_symbol());

		const auto begin = lexer_.current();

		auto [generics, generic_packs] = parse_generic_type_list();

		const auto parameter_begin = lexer_.current();

		expect_and_consume(lexeme_point::get_parentheses_bracket_open_symbol(), "function parameters");

		count_match_recovery_stop_on_token<true>(lexeme_point::token_type::right_arrow);

		temporary_stack params{scratch_annotations_};
		temporary_stack names{scratch_optional_argument_names_};

		ast_type_pack* vararg_annotation = nullptr;

		if (not lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_close_symbol())) { vararg_annotation = parse_type_list(params, names); }

		expect_match_and_consume(lexeme_point::get_parentheses_bracket_close_symbol(), parameter_begin, true);

		count_match_recovery_stop_on_token<false>(lexeme_point::token_type::right_arrow);

		auto param_types = put_object_to_allocator<ast_type_list::types_list_type>(params);

		// Not a function at all. Just a parenthesized type. Or maybe a type pack with a single element
		if (params.size() == 1 && not vararg_annotation && monomorphic && not lexer_.current().is_any_type_of(lexeme_point::token_type::right_arrow))
		{
			if (allow_pack) { return allocator_.new_object<ast_type_pack_explicit>(begin.get_location(), ast_type_list{param_types, nullptr}); }
			return params.bottom();
		}

		if (not lexer_.current().is_any_type_of(lexeme_point::token_type::right_arrow) && monomorphic && allow_pack) { return allocator_.new_object<ast_type_pack_explicit>(begin.get_location(), ast_type_list{param_types, vararg_annotation}); }

		auto param_names = put_object_to_allocator<ast_type_function::argument_names_type>(names);

		return parse_function_type_annotation_tail(begin, generics, generic_packs, param_types, param_names, vararg_annotation);
	}
}
