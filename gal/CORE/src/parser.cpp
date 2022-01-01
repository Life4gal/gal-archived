#include <ast/parser.hpp>

#include <ast/lexer.hpp>
#include <charconv>

namespace gal::ast
{
	template<typename R, typename T, template <typename> class Container>
	R parser::put_object_into_allocator(const Container<T>& container)
	{
		auto result = R{static_cast<T*>(allocator_.allocate(sizeof(T) * container.size())), container.size()};

		std::uninitialized_copy(container.begin(), container.end(), result.data());

		return result;
	}

	template<typename R, typename T>
	R parser::put_object_into_allocator(T data)
	{
		// auto result = R{static_cast<T*>(allocator_.allocate(sizeof(T) * typename R::size_type{1})), typename R::size_type{1}};
		//
		// std::construct_at(result.data(), data);
		//
		// return result;

		// // todo
		(void)data;
		(void)this;
		return R{};
	}

	gal_string_type parser::put_object_into_allocator(const std::string& data) const
	{
		const gal_string_type result{static_cast<gal_string_type::value_type*>(allocator_.allocate(sizeof(gal_string_type::value_type) * data.size())), data.size()};

		std::uninitialized_copy(data.begin(), data.end(), result.data());

		return result;
	}

	parser::parser(const ast_name buffer, ast_name_table& name_table, ast_allocator& allocator, parse_options options)
		: options_{options},
		  lexer_{buffer, name_table},
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

		return allocator_.new_object<ast_statement_block>(utils::location{previous_position, lexer_.current().get_location().begin}, put_object_into_allocator<ast_statement_block::block_body_type>(body));
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

		return report_statement_error(expression->get_location(), put_object_into_allocator<ast_statement_error::error_expressions_type>(expression), {}, "Incomplete statement: expected assignment or a function call.");
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

		if (function_stack_.back().is_root()) { return report_statement_error(begin, {}, put_object_into_allocator<ast_statement_error::error_statements_type>(ret), "break statement must be inside a loop."); }

		return ret;
	}

	ast_statement* parser::parse_continue(utils::location begin)
	{
		auto* ret = allocator_.new_object<ast_statement_continue>(begin);

		if (function_stack_.back().is_root()) { return report_statement_error(begin, {}, put_object_into_allocator<ast_statement_error::error_statements_type>(ret), "continue statement must be inside a loop."); }

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
				put_object_into_allocator<ast_statement_for_in::var_locals_type>(vars),
				put_object_into_allocator<ast_statement_for_in::value_expressions_type>(values),
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
				put_object_into_allocator<ast_statement_local::var_locals_type>(vars),
				put_object_into_allocator<ast_statement_local::value_expressions_type>(values),
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
				put_object_into_allocator<ast_statement_return::expression_list_type>(list));
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
				ast_type_list{put_object_into_allocator<ast_array<ast_type*>>(vars), vararg_annotation},
				put_object_into_allocator<ast_type_function::argument_names_type>(var_names),
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
					ast_type_list{put_object_into_allocator<ast_array<ast_type*>>(vars), vararg_annotation},
					put_object_into_allocator<ast_statement_declare_function::arguments_type>(var_names),
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
					put_object_into_allocator<ast_statement_declare_class::class_properties_type>(properties));
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
		if (not initial->is_lvalue()) { initial = report_expression_error(initial->get_location(), put_object_into_allocator<ast_expression_error::error_expressions_type>(initial), "Assigned expression must be a variable or a field"); }

		temporary_stack vars{scratch_expressions_};
		vars.push(initial);

		while (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol()))
		{
			next_lexeme_point();// ,

			auto* expression = parse_primary_expression(false);

			if (not expression->is_lvalue()) { expression = report_expression_error(expression->get_location(), put_object_into_allocator<ast_expression_error::error_expressions_type>(expression), "Assigned expression must be a variable or a field"); }

			vars.push(expression);
		}

		expect_and_consume(lexeme_point::get_assignment_symbol(), "assignment");

		temporary_stack values{scratch_expression_utils_};
		parse_expression_list(values);

		return allocator_.new_object<ast_statement_assign>(
				make_longest_line(initial->get_location(), values.top()->get_location()),
				put_object_into_allocator<ast_statement_assign::var_expressions_type>(vars),
				put_object_into_allocator<ast_statement_assign::value_expressions_type>(values));
	}

	ast_statement* parser::parse_compound_assignment(ast_expression* initial, ast_expression_binary::operand_type operand)
	{
		if (not initial->is_lvalue()) { initial = report_expression_error(initial->get_location(), put_object_into_allocator<ast_expression_error::error_expressions_type>(initial), "Assigned expression must be a variable or a field"); }

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
						put_object_into_allocator<ast_expression_function::args_locals_type>(vars),
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

			return {result_location, {put_object_into_allocator<ast_type_list::types_list_type>(result), vararg_annotation}};
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
						{put_object_into_allocator<ast_type_list::types_list_type>(return_type), vararg_annotation}
				};
			}

			return {
					loc,
					{put_object_into_allocator<ast_type_list::types_list_type>(result), vararg_annotation}};
		}

		const auto types = put_object_into_allocator<ast_type_list::types_list_type>(result);
		auto names = put_object_into_allocator<ast_type_function::argument_names_type>(result_names);

		temporary_stack fallback_return_types{scratch_annotations_};
		fallback_return_types.push(parse_function_type_annotation_tail(begin, {}, {}, types, names, vararg_annotation));

		return {
				make_longest_line(loc, fallback_return_types.bottom()->get_location()),
				{put_object_into_allocator<ast_type_list::types_list_type>(fallback_return_types), vararg_annotation}
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

		const auto param_types = put_object_into_allocator<ast_type_list::types_list_type>(params);

		// Not a function at all. Just a parenthesized type. Or maybe a type pack with a single element
		if (params.size() == 1 && not vararg_annotation && monomorphic && not lexer_.current().is_any_type_of(lexeme_point::token_type::right_arrow))
		{
			if (allow_pack) { return allocator_.new_object<ast_type_pack_explicit>(begin.get_location(), ast_type_list{param_types, nullptr}); }
			return params.bottom();
		}

		if (not lexer_.current().is_any_type_of(lexeme_point::token_type::right_arrow) && monomorphic && allow_pack) { return allocator_.new_object<ast_type_pack_explicit>(begin.get_location(), ast_type_list{param_types, vararg_annotation}); }

		auto param_names = put_object_into_allocator<ast_type_function::argument_names_type>(names);

		return parse_function_type_annotation_tail(begin, generics, generic_packs, param_types, param_names, vararg_annotation);
	}

	ast_type* parser::parse_function_type_annotation_tail(const lexeme_point& begin, generic_names_type generics, generic_names_type generic_packs, const ast_array<ast_type*>& params, ast_type_function::argument_names_type& param_names, ast_type_pack* vararg_annotation)
	{
		increase_recursion_counter("type annotation");

		// Users occasionally write '()' as the 'unit' type when they actually want to use 'null', here we'll try to give a more specific error
		if (not lexer_.current().is_any_type_of(lexeme_point::token_type::right_arrow) && generics.empty() && generic_packs.empty() && params.empty())
		{
			report(make_longest_line(begin.get_location(), lexer_.previous_location()), std_format::format("Expected '->' after '{}{}' when parsing function type; did you mean '{}'?", lexeme_point::get_parentheses_bracket_open_symbol(), lexeme_point::get_parentheses_bracket_close_symbol(), keyword_null));

			return allocator_.new_object<ast_type_reference>(
					begin.get_location(),
					name_null_,
					std::nullopt);
		}

		expect_and_consume(lexeme_point::token_type::right_arrow, "function type");

		const auto [end_loc, return_type_list] = parse_return_type_annotation();

		return allocator_.new_object<ast_type_function>(
				make_longest_line(begin.get_location(), end_loc),
				generics,
				generic_packs,
				ast_type_list{params, vararg_annotation},
				param_names,
				return_type_list);
	}

	ast_type* parser::parse_table_type_annotation()
	{
		increase_recursion_counter("type annotation");

		temporary_stack properties{scratch_table_properties_};
		ast_type_table::ast_table_indexer* indexer = nullptr;

		const auto begin = lexer_.current().get_location();

		const auto match_brace = lexer_.current();
		expect_and_consume(lexeme_point::get_curly_bracket_open_symbol(), "table type");

		while (not lexer_.current().is_any_type_of(lexeme_point::get_curly_bracket_close_symbol()))
		{
			if (lexer_.current().is_any_type_of(lexeme_point::get_square_bracket_open_symbol()))
			{
				// maybe we don't need to parse the entire bad indexer...
				// however, we either have { or [ to lint, not the entire table type or the bad indexer.
				auto* bad_indexer = parse_table_indexer_annotation();

				if (indexer)
				{
					// we lose all additional indexer expressions from the AST after error recovery here
					report(bad_indexer->loc, "Cannot have more than one table indexer");
				}
				else { indexer = bad_indexer; }
			}
			else if (properties.empty() && not indexer && not(lexer_.current().is_any_type_of(lexeme_point::token_type::name) && lexer_.peek_next().is_any_type_of(lexeme_point::get_colon_symbol())))
			{
				auto* type = parse_type_annotation();

				// array-like table type: {T} de-sugars into {[number]: T}
				auto* index = allocator_.new_object<ast_type_reference>(
						type->get_location(),
						name_number_,
						std::nullopt);

				indexer = allocator_.new_object<ast_type_table::ast_table_indexer>(
						index,
						type,
						type->get_location());

				break;
			}
			else
			{
				auto name = parse_name_optional("table field");

				if (not name.has_value()) { break; }

				expect_and_consume(lexeme_point::get_colon_symbol(), "table field");

				auto* type = parse_type_annotation();

				properties.emplace(name->name, name->loc, type);
			}

			if (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol(), lexeme_point::get_semicolon_symbol()))
			{
				next_lexeme_point();// , or ;
			}
			else { if (not lexer_.current().is_any_type_of(lexeme_point::get_curly_bracket_close_symbol())) { break; } }
		}

		auto end = lexer_.current().get_location();

		if (not expect_match_and_consume(lexeme_point::get_curly_bracket_close_symbol(), match_brace)) { end = lexer_.previous_location(); }

		return allocator_.new_object<ast_type_table>(
				make_longest_line(begin, end),
				put_object_into_allocator<ast_type_table::table_properties_type>(properties),
				indexer);
	}

	ast_type_or_pack parser::parse_simple_type_annotation(const bool allow_pack)
	{
		increase_recursion_counter("type annotation");

		auto begin = lexer_.current().get_location();

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_null))
		{
			next_lexeme_point();// null
			return allocator_.new_object<ast_type_reference>(
					begin,
					name_null_,
					std::nullopt);
		}

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::name))
		{
			auto name = parse_name("type name");

			std::optional<ast_name> prefix;

			if (lexer_.current().is_any_type_of(lexeme_point::get_dot_symbol()))
			{
				const auto dot_pos = lexer_.current().get_location().begin;
				next_lexeme_point();// .

				prefix = name.name;
				name = parse_index_name("field name", dot_pos);
			}
			else if (name.name == lexeme_point::get_typeof_keyword())
			{
				const auto typeof_begin = lexer_.current();
				expect_and_consume(lexeme_point::get_parentheses_bracket_open_symbol(), "typeof type");

				auto* expression = parse_expression();

				const auto end = lexer_.current().get_location();

				expect_match_and_consume(lexeme_point::get_parentheses_bracket_close_symbol(), typeof_begin);

				return allocator_.new_object<ast_type_typeof>(
						make_longest_line(begin, end),
						expression
						);
			}

			std::optional<decltype(parse_type_params())> parameters;

			if (lexer_.current().is_any_type_of(lexeme_point::get_less_than_symbol())) { parameters = parse_type_params(); }

			const auto end = lexer_.previous_location();

			return allocator_.new_object<ast_type_reference>(
					make_longest_line(begin, end),
					name.name,
					prefix,
					parameters
					);
		}

		if (lexer_.current().is_any_type_of(lexeme_point::get_curly_bracket_open_symbol())) { return parse_table_type_annotation(); }

		if (lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_open_symbol(), lexeme_point::get_less_than_symbol())) { parse_function_type_annotation(allow_pack); }

		// For a missing type annotation, capture 'space' between last token and the next one
		const utils::location loc{lexer_.previous_location().end, lexer_.current().get_location().begin};

		return report_type_annotation_error(loc, {}, true, std_format::format("Expected type, but got {}", lexer_.current().to_string()));
	}

	ast_type_or_pack parser::parse_type_or_pack_annotation()
	{
		const auto previous_recursion_count = recursion_counter_;

		increase_recursion_counter("type annotation");

		const auto begin = lexer_.current().get_location();

		temporary_stack parts{scratch_annotations_};

		auto type_or_pack = parse_simple_type_annotation(true);

		if (type_or_pack.holding<ast_type_pack>()) { return type_or_pack; }

		parts.push(type_or_pack.as<ast_type>());

		recursion_counter_ = previous_recursion_count;

		return parse_type_annotation(parts, begin);
	}

	ast_type* parser::parse_type_annotation(temporary_stack<ast_type*>& parts, utils::location begin)
	{
		gal_assert(not parts.empty());

		increase_recursion_counter("type annotation");

		bool is_union = false;
		bool is_intersection = false;

		auto loc = begin;

		while (true)
		{
			if (lexer_.current().is_any_type_of(lexeme_point::get_bitwise_or_symbol(), lexeme_point::get_bitwise_and_symbol()))
			{
				next_lexeme_point();// | or &

				parts.push(parse_simple_type_annotation(false).as<ast_type>());

				is_union = lexer_.current().is_any_type_of(lexeme_point::get_bitwise_or_symbol());
				is_intersection = lexer_.current().is_any_type_of(lexeme_point::get_bitwise_and_symbol());
			}
			else if (lexer_.current().is_any_type_of(lexeme_point::get_question_mark_symbol()))
			{
				const auto current = lexer_.current().get_location();
				next_lexeme_point();// ?

				parts.push(allocator_.new_object<ast_type_reference>(
						current,
						name_null_,
						std::nullopt));
				is_union = true;
			}
			else { break; }
		}

		if (parts.size() == 1) { return parts.bottom(); }

		if (is_union && is_intersection)
		{
			return report_type_annotation_error(
					make_longest_line(begin, parts.top()->get_location()),
					put_object_into_allocator<ast_type_error::error_types_type>(parts),
					false,
					std_format::format("Mixing union('{}') and intersection('{}') types is not allowed; consider wrapping in parentheses.", lexeme_point::get_bitwise_or_symbol(), lexeme_point::get_bitwise_and_symbol()));
		}

		loc.end = parts.top()->get_location().end;

		if (is_union) { return allocator_.new_object<ast_type_union>(loc, put_object_into_allocator<ast_type_union::union_types_type>(parts)); }

		if (is_intersection) { return allocator_.new_object<ast_type_intersection>(loc, put_object_into_allocator<ast_type_intersection::intersection_types_type>(parts)); }

		gal_assert(false, "Composite type was not an intersection or union.");
		throw parse_error{begin, "Composite type was not an intersection or union."};
	}

	ast_type* parser::parse_type_annotation()
	{
		const auto previous_recursion_count = recursion_counter_;

		increase_recursion_counter("type annotation");

		const auto begin = lexer_.current().get_location();

		temporary_stack parts{scratch_annotations_};
		parts.push(parse_simple_type_annotation(false).as<ast_type>());

		recursion_counter_ = previous_recursion_count;

		return parse_type_annotation(parts, begin);
	}


	ast_type_pack* parser::parse_type_pack_annotation()
	{
		// variadic: ...T
		if (lexer_.current().is_any_type_of(lexeme_point::token_type::ellipsis))
		{
			const auto begin = lexer_.current().get_location();

			next_lexeme_point();// ...

			auto* vararg_type = parse_type_annotation();
			return allocator_.new_object<ast_type_pack_variadic>(make_longest_line(begin, vararg_type->get_location()), vararg_type);
		}

		// generic: a...
		if (lexer_.current().is_any_type_of(lexeme_point::token_type::name) && lexer_.peek_next().is_any_type_of(lexeme_point::token_type::ellipsis))
		{
			auto [name, loc] = parse_name("generic name");
			const auto end = lexer_.current().get_location();

			// This will not fail because of the peek_next guard.
			expect_and_consume(lexeme_point::token_type::ellipsis, "generic type pack annotation");
			return allocator_.new_object<ast_type_pack_generic>(make_longest_line(loc, end), name);
		}

		// No type pack annotation exists here.
		return nullptr;
	}

	ast_type_pack* parser::parse_variadic_argument_annotation()
	{
		// generic: a...
		if (lexer_.current().is_any_type_of(lexeme_point::token_type::name) && lexer_.peek_next().is_any_type_of(lexeme_point::token_type::ellipsis))
		{
			auto [name, loc] = parse_name("generic name");
			const auto end = lexer_.current().get_location();

			// This will not fail because of the peek_next guard.
			expect_and_consume(lexeme_point::token_type::ellipsis, "generic type pack annotation");
			return allocator_.new_object<ast_type_pack_generic>(make_longest_line(loc, end), name);
		}

		// variadic: ...T
		auto* variadic_annotation = parse_type_annotation();
		return allocator_.new_object<ast_type_pack_variadic>(variadic_annotation->get_location(), variadic_annotation);
	}


	ast_expression* parser::parse_expression(ast_expression_binary::operand_priority_type limit)
	{
		const auto previous_recursion_count = recursion_counter_;

		// this handles recursive calls to parse sub-expression/ parse expression
		increase_recursion_counter("expression");

		const auto begin = lexer_.current().get_location();

		ast_expression* expression;

		auto unary_operand = lexer_.current().to_unary_operand();

		if (not unary_operand.has_value()) { unary_operand = lexer_.current().check_unary_operand([this](const utils::location loc, std::string message) { report(loc, std::move(message)); }); }

		if (unary_operand.has_value())
		{
			next_lexeme_point();// pass this unary operand

			auto* sub_expression = parse_expression(ast_expression_binary::unary_operand_priority);

			expression = allocator_.new_object<ast_expression_unary>(make_longest_line(begin, sub_expression->get_location()), unary_operand.value(), sub_expression);
		}
		else { expression = parse_assertion_expression(); }

		// expand while operators have priorities higher than `limit'
		auto binary_operand = lexer_.current().to_binary_operand();

		if (not binary_operand.has_value()) { binary_operand = lexer_.current().check_binary_operand([this](const utils::location loc, std::string message) { report(loc, std::move(message)); }); }

		while (binary_operand.has_value())
		{
			const auto [left, right] = ast_expression_binary::get_priority(binary_operand.value());

			if (left <= limit) { break; }

			next_lexeme_point();

			// read sub-expression with higher priority
			auto* next = parse_expression(right);

			expression = allocator_.new_object<ast_expression_binary>(make_longest_line(begin, next->get_location()), binary_operand.value(), expression, next);
			binary_operand = lexer_.current().to_binary_operand();

			if (not binary_operand.has_value()) { binary_operand = lexer_.current().check_binary_operand([this](const utils::location loc, std::string message) { report(loc, std::move(message)); }); }

			// note: while the parser isn't recursive here, we're generating recursive structures of unbounded depth
			increase_recursion_counter("expression");
		}

		recursion_counter_ = previous_recursion_count;

		return expression;
	}

	ast_expression* parser::parse_name_expression(const char* context)
	{
		auto name = parse_name_optional(context);

		if (not name.has_value())
		{
			return allocator_.new_object<ast_expression_error>(
					lexer_.current().get_location(),
					ast_expression_error::error_expressions_type{},
					parse_errors_.size() - 1
					);
		}

		if (const auto value = local_map_.find(name->name);
			value != local_map_.end() && value->second)
		{
			auto& [_, local] = *value;

			return allocator_.new_object<ast_expression_local>(
					name->loc,
					local,
					local->function_depth != function_stack_.size() - 1
					);
		}

		return allocator_.new_object<ast_expression_global>(name->loc, name->name);
	}

	ast_expression* parser::parse_prefix_expression()
	{
		if (lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_open_symbol()))
		{
			const auto begin = lexer_.current().get_location();

			const auto match_paren = lexer_.current();

			next_lexeme_point();// (

			auto* expression = parse_expression();

			auto end = lexer_.current().get_location();

			if (not lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_close_symbol()))
			{
				expect_match_and_consume_fail(
						static_cast<lexeme_point::token_type>(lexeme_point::get_parentheses_bracket_close_symbol()),
						match_paren,
						lexer_.current().is_any_type_of(lexeme_point::get_assignment_symbol()) ? std_format::format("; did you mean to use '{}' when defining a table?", lexeme_point::get_curly_bracket_open_symbol()) : ""
						);

				end = lexer_.previous_location();
			}
			else
			{
				next_lexeme_point();// )
			}

			return allocator_.new_object<ast_expression_group>(make_longest_line(begin, end), expression);
		}

		return parse_name_expression("expression");
	}


	ast_expression* parser::parse_primary_expression(const bool as_statement)
	{
		const auto begin = lexer_.current().get_location();

		auto* expression = parse_prefix_expression();

		const auto previous_recursion_count = recursion_counter_;

		while (true)
		{
			if (lexer_.current().is_any_type_of(lexeme_point::get_dot_symbol()))
			{
				const auto operand_pos = lexer_.current().get_location().begin;

				next_lexeme_point();// .

				auto [name, loc] = parse_index_name(nullptr, operand_pos);

				expression = allocator_.new_object<ast_expression_index_name>(
						make_longest_line(begin, loc),
						expression,
						name,
						loc,
						operand_pos,
						lexeme_point::get_dot_symbol());
			}
			else if (lexer_.current().is_any_type_of(lexeme_point::get_square_bracket_open_symbol()))
			{
				const auto match_bracket = lexer_.current();

				next_lexeme_point();// [

				auto* index = parse_expression();

				const auto end = lexer_.current().get_location();

				expect_match_and_consume(lexeme_point::get_square_bracket_close_symbol(), match_bracket);

				expression = allocator_.new_object<ast_expression_index_expression>(
						make_longest_line(begin, end),
						expression,
						index);
			}
			else if (lexer_.current().is_any_type_of(lexeme_point::get_at_symbol()))
			{
				const auto operand_pos = lexer_.current().get_location().begin;

				next_lexeme_point();// @

				auto [name, loc] = parse_index_name("method name", operand_pos);

				auto* function = allocator_.new_object<ast_expression_index_name>(
						make_longest_line(begin, loc),
						expression,
						name,
						loc,
						operand_pos,
						lexeme_point::get_colon_symbol());

				expression = parse_function_arguments(function, true, loc);
			}
			else if (lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_open_symbol()))
			{
				// This error is handled inside 'parse_function_arguments' as well, but for better error recovery we need to break out the current loop here
				if (not as_statement && expression->get_location().end.line != lexer_.current().get_location().begin.line)
				{
					report(lexer_.current().get_location(),
					       std_format::format("Ambiguous syntax: this looks like an argument list for a function call, but could also be a start of "
					                          "new statement; use '{}' to separate statements",
					                          lexeme_point::get_semicolon_symbol())
							);

					break;
				}

				expression = parse_function_arguments(expression, false, {});
			}
			else if (lexer_.current().is_any_type_of(lexeme_point::get_curly_bracket_open_symbol(), lexeme_point::token_type::raw_string, lexeme_point::token_type::quoted_string)) { expression = parse_function_arguments(expression, false, {}); }
			else { break; }

			// note: while the parser isn't recursive here, we're generating recursive structures of unbounded depth
			increase_recursion_counter("expression");
		}

		recursion_counter_ = previous_recursion_count;

		return expression;
	}

	ast_expression* parser::parse_assertion_expression()
	{
		const auto begin = lexer_.current().get_location();

		auto* expression = parse_simple_expression();

		if (options_.allow_type_annotations && lexer_.current().is_any_type_of(lexeme_point::token_type::double_colon))
		{
			next_lexeme_point();// ::

			auto* annotation = parse_type_annotation();

			return allocator_.new_object<ast_expression_type_assertion>(
					make_longest_line(begin, annotation->get_location()),
					expression,
					annotation);
		}

		return expression;
	}

	ast_expression* parser::parse_simple_expression()
	{
		auto begin = lexer_.current().get_location();

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::number))
		{
			scratch_data_.assign(lexer_.current().get_data_or_name());

			// Remove all internal _
			if (scratch_data_.find(lexeme_point::get_underscore_symbol()) != decltype(scratch_data_)::npos) { std::erase(scratch_data_, lexeme_point::get_underscore_symbol()); }

			gal_number_type value{0};
			const auto [ptr, ec] = std::from_chars(scratch_data_.data(), scratch_data_.data() + scratch_data_.size(), value);

			next_lexeme_point();

			if (ec != std::errc{} || ptr != scratch_data_.data() + scratch_data_.size()) { return report_expression_error(begin, {}, std_format::format("Malformed number: {}", scratch_data_)); }
			return allocator_.new_object<ast_expression_constant_number>(begin, value);
		}

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::raw_string, lexeme_point::token_type::quoted_string)) { return parse_string(); }
		if (lexer_.current().is_any_type_of(lexeme_point::token_type::broken_string))
		{
			next_lexeme_point();
			return report_expression_error(begin, {}, "Malformed string");
		}

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_null))
		{
			next_lexeme_point();// null

			return allocator_.new_object<ast_expression_constant_null>(begin);
		}

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_true, lexeme_point::token_type::keyword_false))
		{
			next_lexeme_point();// true / false

			return allocator_.new_object<ast_expression_constant_boolean>(begin, lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_true));
		}

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::ellipsis))
		{
			next_lexeme_point();// ...

			if (function_stack_.back().vararg) { return allocator_.new_object<ast_expression_varargs>(begin); }
			return report_expression_error(begin, {}, "Cannot use ellipsis('...') outside of a vararg function");
		}

		if (lexer_.current().is_any_type_of(lexeme_point::get_curly_bracket_open_symbol())) { return parse_table_constructor(); }

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_function))
		{
			const auto match_function = lexer_.current();

			next_lexeme_point();// function

			return parse_function_body(false, match_function, {}, {}).first;
		}

		return parse_primary_expression(false);
	}

	ast_expression* parser::parse_function_arguments(ast_expression* function, bool has_self, utils::location self_loc)
	{
		(void)self_loc;

		if (lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_open_symbol()))
		{
			const auto arg_begin = lexer_.current().get_location().end;

			if (function->get_location().end.line != lexer_.current().get_location().begin.line)
				report(lexer_.current().get_location(),
				       std_format::format("Ambiguous syntax: this looks like an argument list for a function call, but could also be a start of new statement; use '{}' to separate statements", lexeme_point::get_semicolon_symbol()));

			const auto match_paren = lexer_.current();

			next_lexeme_point();// (

			temporary_stack args{scratch_expressions_};

			if (not lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_close_symbol())) { parse_expression_list(args); }

			const auto end = lexer_.current().get_location();
			const auto arg_end = end.end;

			expect_match_and_consume(lexeme_point::get_parentheses_bracket_close_symbol(), match_paren);

			return allocator_.new_object<ast_expression_call>(
					make_longest_line(function->get_location(), end),
					function,
					put_object_into_allocator<ast_expression_call::call_args_type>(args),
					has_self,
					utils::location{arg_begin, arg_end});
		}

		if (lexer_.current().is_any_type_of(lexeme_point::get_curly_bracket_open_symbol()))
		{
			const auto arg_begin = lexer_.current().get_location().end;
			auto* expression = parse_table_constructor();
			const auto arg_end = lexer_.previous_location().end;

			return allocator_.new_object<ast_expression_call>(
					make_longest_line(function->get_location(), expression->get_location()),
					function,
					put_object_into_allocator<ast_expression_call::call_args_type>(expression),
					has_self,
					utils::location{arg_begin, arg_end});
		}

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::raw_string, lexeme_point::token_type::quoted_string))
		{
			const auto arg_loc = lexer_.current().get_location();
			auto* expression = parse_string();

			return allocator_.new_object<ast_expression_call>(
					make_longest_line(function->get_location(), expression->get_location()),
					function,
					put_object_into_allocator<ast_expression_call::call_args_type>(expression),
					has_self,
					arg_loc);
		}

		if (has_self && lexer_.current().get_location().begin.line != function->get_location().end.line)
		{
			return report_expression_error(
					function->get_location(),
					put_object_into_allocator<ast_expression_error::error_expressions_type>(function),
					std_format::format("Expected function call arguments after '{}'", lexeme_point::get_parentheses_bracket_open_symbol()));
		}

		return report_expression_error(
				utils::location{function->get_location().begin, lexer_.current().get_location().begin},
				put_object_into_allocator<ast_expression_error::error_expressions_type>(function),
				std_format::format("Expected '{}', '{}' or <string> when parsing function call, got {}", lexeme_point::get_parentheses_bracket_open_symbol(), lexeme_point::get_curly_bracket_open_symbol(), lexer_.current().to_string()));
	}

	ast_expression* parser::parse_table_constructor()
	{
		temporary_stack items{scratch_items_};

		const auto begin = lexer_.current().get_location();

		const auto match_brace = lexer_.current();

		expect_and_consume(lexeme_point::get_curly_bracket_open_symbol(), "table literal");

		while (not lexer_.current().is_any_type_of(lexeme_point::get_curly_bracket_close_symbol()))
		{
			if (lexer_.current().is_any_type_of(lexeme_point::get_square_bracket_open_symbol()))
			{
				const auto match_bracket = lexer_.current();

				next_lexeme_point();// [

				auto* key = parse_expression();

				expect_match_and_consume(lexeme_point::get_square_bracket_close_symbol(), match_bracket);

				expect_and_consume(lexeme_point::get_assignment_symbol(), "table field");

				auto* value = parse_expression();

				items.emplace(ast_expression_table::item_type::general, key, value);
			}
			else if (lexer_.current().is_any_type_of(lexeme_point::token_type::name) && lexer_.peek_next().is_any_type_of(lexeme_point::get_assignment_symbol()))
			{
				auto [name, loc] = parse_name("table field");

				expect_and_consume(lexeme_point::get_assignment_symbol(), "table field");

				auto* value = parse_expression();

				items.emplace(ast_expression_table::item_type::record, allocator_.new_object<ast_expression_constant_string>(loc, name), value);
			}
			else
			{
				auto* expression = parse_expression();

				items.emplace(ast_expression_table::item_type::list, nullptr, expression);
			}

			if (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol(), lexeme_point::get_semicolon_symbol()))
			{
				next_lexeme_point();// , or ;
			}
			else { if (not lexer_.current().is_any_type_of(lexeme_point::get_curly_bracket_close_symbol())) { break; } }
		}

		auto end = lexer_.current().get_location();

		if (not expect_match_and_consume(lexeme_point::get_curly_bracket_close_symbol(), match_brace)) { end = lexer_.previous_location(); }

		return allocator_.new_object<ast_expression_table>(
				make_longest_line(begin, end),
				put_object_into_allocator<ast_expression_table::items_type>(items));
	}

	ast_expression* parser::parse_if_else_expression()
	{
		const auto begin = lexer_.current().get_location();

		next_lexeme_point();// if or elif

		auto* condition = parse_expression();

		bool has_then = expect_and_consume(lexeme_point::token_type::keyword_then, "if then else expression");

		auto* true_branch = parse_expression();
		bool has_else;
		decltype(true_branch) false_branch;

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::keyword_elif))
		{
			const auto previous_recursion_count = recursion_counter_;

			increase_recursion_counter("expression");

			has_else = true;
			false_branch = parse_if_else_expression();

			recursion_counter_ = previous_recursion_count;
		}
		else
		{
			has_else = expect_and_consume(lexeme_point::token_type::keyword_else, "if then else expression");
			false_branch = parse_expression();
		}

		const auto end = false_branch->get_location();

		return allocator_.new_object<ast_expression_if_else>(
				make_longest_line(begin, end),
				has_then,
				has_else,
				condition,
				true_branch,
				false_branch);
	}

	std::optional<parser::parse_name_result> parser::parse_name_optional(const char* context)
	{
		if (lexer_.current().is_any_type_of(lexeme_point::token_type::name))
		{
			const parse_name_result result{lexer_.current().get_data_or_name(), lexer_.current().get_location()};

			next_lexeme_point();// name

			return result;
		}

		report_name_error(context);

		return std::nullopt;
	}

	parser::parse_name_result parser::parse_name(const char* context)
	{
		if (const auto name = parse_name_optional(context); name.has_value()) { return name.value(); }

		return {name_error_, {lexer_.current().get_location().begin, lexer_.current().get_location().begin}};
	}


	parser::parse_name_result parser::parse_index_name(const char* context, utils::position previous)
	{
		if (const auto name = parse_name_optional(context); name.has_value()) { return name.value(); }

		// If we have a reserved keyword next at the same line, assume it's an incomplete name
		if (lexer_.current().is_any_keyword() && lexer_.peek_next().get_location().begin.line == previous.line)
		{
			const parse_name_result result{lexer_.current().get_data_or_name(), lexer_.current().get_location()};

			next_lexeme_point();// keyword

			return result;
		}

		return {name_error_, {lexer_.current().get_location().begin, lexer_.current().get_location().begin}};
	}

	std::pair<generic_names_type, generic_names_type> parser::parse_generic_type_list()
	{
		temporary_stack names{scratch_names_};
		temporary_stack name_packs{scratch_pack_names_};

		if (lexer_.current().is_any_type_of(lexeme_point::get_less_than_symbol()))
		{
			const auto begin = lexer_.current();

			next_lexeme_point();// <

			bool has_pack = false;
			while (true)
			{
				auto name = parse_name().name;
				if (lexer_.current().is_any_type_of(lexeme_point::token_type::ellipsis))
				{
					has_pack = true;

					next_lexeme_point();// ...

					name_packs.push(name);
				}
				else
				{
					if (has_pack) { report(lexer_.current().get_location(), "Generic types come before generic type packs"); }

					names.push(name);
				}

				if (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol()))
				{
					next_lexeme_point();// ,
				}
				else { break; }
			}

			expect_match_and_consume(lexeme_point::get_greater_than_symbol(), begin);
		}

		return {put_object_into_allocator<generic_names_type>(names), put_object_into_allocator<generic_names_type>(name_packs)};
	}

	ast_array<ast_type_or_pack> parser::parse_type_params()
	{
		temporary_stack parameters{scratch_type_or_pack_annotations_};

		if (lexer_.current().is_any_type_of(lexeme_point::get_less_than_symbol()))
		{
			const auto begin = lexer_.current();

			next_lexeme_point();// <

			while (true)
			{
				if (lexer_.current().is_any_type_of(lexeme_point::token_type::ellipsis) || (lexer_.current().is_any_type_of(lexeme_point::token_type::name) && lexer_.peek_next().is_any_type_of(lexeme_point::token_type::ellipsis))) { parameters.emplace(parse_type_pack_annotation()); }
				else if (lexer_.current().is_any_type_of(lexeme_point::get_parentheses_bracket_open_symbol())) { parameters.push(parse_type_or_pack_annotation()); }
				else if (lexer_.current().is_any_type_of(lexeme_point::get_greater_than_symbol()) && parameters.empty()) { break; }
				else { parameters.emplace(parse_type_annotation()); }

				if (lexer_.current().is_any_type_of(lexeme_point::get_comma_symbol()))
				{
					next_lexeme_point();// ,
				}
				else { break; }
			}

			expect_match_and_consume(lexeme_point::get_greater_than_symbol(), begin);
		}

		return put_object_into_allocator<ast_array<ast_type_or_pack>>(parameters);
	}

	std::optional<gal_string_type> parser::parse_char_array()
	{
		gal_assert(lexer_.current().is_any_type_of(lexeme_point::token_type::raw_string, lexeme_point::token_type::quoted_string));

		scratch_data_.assign(lexer_.current().get_data_or_name());

		if (lexer_.current().is_any_type_of(lexeme_point::token_type::raw_string)) { lexer::write_multi_line_string(scratch_data_); }
		else
		{
			if (not lexer::write_quoted_string(scratch_data_))
			{
				next_lexeme_point();
				return std::nullopt;
			}
		}

		auto ret = put_object_into_allocator(scratch_data_);
		next_lexeme_point();
		return std::make_optional(ret);
	}

	ast_expression* parser::parse_string()
	{
		auto loc = lexer_.current().get_location();
		if (auto value = parse_char_array();
			value.has_value()) { return allocator_.new_object<ast_expression_constant_string>(loc, value.value()); }

		return report_expression_error(loc, {}, "String literal contains malformed escape sequence");
	}

	ast_local* parser::push_local(const parse_name_binding_result& binding)
	{
		const auto& [name, annotation] = binding;

		auto& local = local_map_[name.name];

		// todo: maybe something wrong about ast_local::shadow?
		local = allocator_.new_object<ast_local>(name.name, name.loc, local, function_stack_.size() - 1, function_stack_.back().loop_depth, annotation);

		local_stack_.push_back(local);

		return local;
	}

	constexpr parser::locals_stack_size_type parser::save_locals() const { return local_stack_.size(); }

	void parser::restore_locals(const locals_stack_size_type offset)
	{
		for (auto i = local_stack_.size(); i > offset; --i)
		{
			const auto* local = local_stack_[i - 1];

			local_map_[local->name] = local->shadow;
		}

		local_stack_.resize(offset);
	}

	bool parser::expect_and_consume(lexeme_point::token_underlying_type type, const char* context) { return expect_and_consume(static_cast<lexeme_point::token_type>(type), context); }


	bool parser::expect_and_consume(lexeme_point::token_type type, const char* context)
	{
		if (not lexer_.current().is_any_type_of(type))
		{
			expect_and_consume_fail(type, context);

			// check if this is an extra token and the expected token is next
			if (lexer_.peek_next().is_any_type_of(type))
			{
				// skip invalid and consume expected
				next_lexeme_point();
				next_lexeme_point();
			}

			return false;
		}

		next_lexeme_point();
		return true;
	}

	void parser::expect_and_consume_fail(lexeme_point::token_type type, const char* context)
	{
		auto type_string = lexeme_point{type, {}}.to_string();
		auto current_point_string = lexer_.current().to_string();

		report(
				lexer_.current().get_location(),
				std_format::format(
						"Expected {}{}{}, but got {}",
						type_string,
						context ? " when parsing " : "",
						context ? context : "",
						current_point_string
						)
				);
	}

	bool parser::expect_match_and_consume(lexeme_point::token_underlying_type type, const lexeme_point& begin, const bool search_for_missing) { return expect_match_and_consume(static_cast<lexeme_point::token_type>(type), begin, search_for_missing); }

	bool parser::expect_match_and_consume(const lexeme_point::token_type type, const lexeme_point& begin, const bool search_for_missing)
	{
		if (not lexer_.current().is_any_type_of(type))
		{
			expect_match_and_consume_fail(type, begin);

			if (search_for_missing)
			{
				// previous location is taken because 'current' lexeme_point is already the next token
				const auto current_line = lexer_.previous_location().end.line;

				// search to the end of the line for expected token
				// we will also stop if we hit a token that can be handled by parsing function above the current one
				auto current_type = lexer_.current().get_type();

				while (current_line == lexer_.current().get_location().begin.line && current_type != type && get_match_recovery_stop_on_token(current_type) == 0)
				{
					next_lexeme_point();

					current_type = lexer_.current().get_type();
				}

				if (current_type == type)
				{
					next_lexeme_point();

					return true;
				}
			}
			else
			{
				// check if this is an extra token and the expected token is next
				if (lexer_.peek_next().is_any_type_of(type))
				{
					// skip invalid and consume expected
					next_lexeme_point();
					next_lexeme_point();

					return true;
				}
			}

			return false;
		}

		next_lexeme_point();
		return true;
	}


	void parser::expect_match_and_consume_fail(const lexeme_point::token_type type, const lexeme_point& begin, const std::string& extra)
	{
		auto type_string = lexeme_point{type, {}}.to_string();

		report(
				lexer_.current().get_location(),
				std_format::format(
						"Expected {} (to close {} at {} {}), got {}{}",
						type_string,
						begin.to_string(),
						lexer_.current().get_location().begin.line == begin.get_location().begin.line ? "column" : "line",
						lexer_.current().get_location().begin.line == begin.get_location().begin.line ? begin.get_location().begin.column + 1 : begin.get_location().begin.line + 1,
						lexer_.current().to_string(),
						extra)
				);
	}

	bool parser::expect_match_end_and_consume(lexeme_point::token_type type, const lexeme_point& begin)
	{
		if (not lexer_.current().is_any_type_of(type))
		{
			expect_match_end_and_consume_fail(type, begin);

			// check if this is an extra token and the expected token is next
			if (lexer_.peek_next().is_any_type_of(type))
			{
				// skip invalid and consume expected
				next_lexeme_point();
				next_lexeme_point();

				return true;
			}

			return false;
		}

		// If the token matches on a different line and a different column, it suggests misleading indentation
		// This can be used to pinpoint the problem location for a possible future *actual* mismatch
		const auto [current_line, current_column] = lexer_.current().get_location().begin;
		const auto [line, column] = begin.get_location().begin;
		if (
			current_line != line &&
			current_column != column &&
			// Only replace the previous suspect with more recent suspects
			end_mismatch_suspect_.get_location().begin.line < line) { end_mismatch_suspect_ = begin; }

		next_lexeme_point();

		return true;
	}

	void parser::expect_match_end_and_consume_fail(lexeme_point::token_type type, const lexeme_point& begin)
	{
		if (not end_mismatch_suspect_.is_end_point() && end_mismatch_suspect_.get_location().begin.line > begin.get_location().begin.line) { expect_match_and_consume_fail(type, begin, std_format::format("; did you forget to close {} at line {}?", end_mismatch_suspect_.to_string(), end_mismatch_suspect_.get_location().begin.line + 1)); }
		else { expect_match_and_consume_fail(type, begin, ""); }
	}

	void parser::increase_recursion_counter(const char* context)
	{
		++recursion_counter_;

		if (recursion_counter_ > max_recursion_size) { throw parse_error{lexer_.current().get_location(), std_format::format("Exceeded allowed recursion depth: {}; simplify your {} to make the code compile", max_recursion_size, context)}; }
	}

	void parser::report(utils::location loc, std::string message)
	{
		// To reduce number of errors reported to user for incomplete statements, we skip multiple errors at the same location
		if (not parse_errors_.empty() && loc == parse_errors_.back().where_error()) { return; }

		parse_errors_.emplace_back(loc, std::move(message));

		if (parse_errors_.size() >= max_parse_error_size) { throw parse_error{loc, std_format::format("Reached error limit: {}", max_parse_error_size)}; }
	}

	void parser::report_name_error(const char* context)
	{
		report(lexer_.current().get_location(),
		       std_format::format(
				       "Expected identifier{}{}, but got {}",
				       context ? " when parsing " : "",
				       context ? context : "",
				       lexer_.current().to_string()
				       ));
	}

	ast_statement_error* parser::report_statement_error(
			utils::location loc,
			ast_statement_error::error_expressions_type expressions,
			ast_statement_error::error_statements_type statements,
			std::string message)
	{
		report(loc, std::move(message));

		return allocator_.new_object<ast_statement_error>(loc, expressions, statements, parse_errors_.size() - 1);
	}


	ast_expression_error* parser::report_expression_error(
			utils::location loc,
			ast_expression_error::error_expressions_type expressions,
			std::string message)
	{
		report(loc, std::move(message));

		return allocator_.new_object<ast_expression_error>(loc, expressions, parse_errors_.size() - 1);
	}

	ast_type_error* parser::report_type_annotation_error(utils::location loc, ast_type_error::error_types_type types, bool is_missing, std::string message)
	{
		report(loc, std::move(message));

		return allocator_.new_object<ast_type_error>(loc, types, is_missing, parse_errors_.size() - 1);
	}


	const lexeme_point& parser::next_lexeme_point()
	{
		if (options_.capture_comments)
		{
			while (true)
			{
				// Subtlety: Broken comments are weird because we record them as comments AND pass them to the parser as a lexeme.
				// The parser will turn this into a proper syntax error.
				if (const auto& lexeme_point = lexer_.next(false);
					lexeme_point.is_any_type_of(lexeme_point::token_type::broken_comment) || lexeme_point.is_comment()) { comment_locations_.emplace_back(lexeme_point.get_type(), lexeme_point.get_location()); }
				else { return lexeme_point; }
			}
		}

		return lexer_.next();
	}

	parse_result parser::parse(const ast_name buffer, ast_name_table& name_table, ast_allocator& allocator, const parse_options options)
	{
		// todo: timer?

		parser p{buffer, name_table, allocator, options};

		try
		{
			std::vector<std::string> hot_comments;

			while (p.lexer_.current().is_any_type_of(lexeme_point::token_type::broken_comment) || p.lexer_.current().is_comment())
			{
				if (auto data = p.lexer_.current().get_data_or_name();
					not data.empty() && data[0] == lexeme_point::get_not_symbol())
				{
					hot_comments.emplace_back(
							data.begin() + 1,
							std::ranges::find_if_not(data.rbegin(), data.rend(), [](const auto c) { return utils::is_whitespace(c); }).base());
				}

				const auto type = p.lexer_.current().get_type();
				const auto loc = p.lexer_.current().get_location();

				p.lexer_.next();

				if (options.capture_comments) { p.comment_locations_.emplace_back(type, loc); }
			}

			p.lexer_.set_skip_comment(true);

			auto* root = p.parse_chunk();

			return {root, std::move(hot_comments), std::move(p.parse_errors_), std::move(p.comment_locations_)};
		}
		catch (parse_error& error)
		{
			// when catching a fatal error, append it to the list of non-fatal errors and return
			p.parse_errors_.push_back(error);

			return {nullptr, {}, std::move(p.parse_errors_), {}};
		}
	}
}
