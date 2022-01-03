#include <compiler/compiler.hpp>
#include <variant>
#include <utils/hash_container.hpp>
#include <utils/hash.hpp>
#include <ast/parser.hpp>
#include <compiler/bytecode_builder.hpp>

namespace gal::compiler
{
	[[nodiscard]] constexpr operands unary_operand_to_operands(const ast::ast_expression_unary::operand_type operand) noexcept
	{
		switch (operand)
		{
				using enum ast::ast_expression_unary::operand_type;
			case unary_plus: { return operands::unary_plus; }
			case unary_minus: { return operands::unary_minus; }
			case unary_not: { return operands::unary_not; }
			default: // NOLINT(clang-diagnostic-covered-switch-default)
			{
				UNREACHABLE();
				gal_assert(false, "Unexpected unary operation!");
				return operands::nop;
			}
		}
	}

	[[nodiscard]] constexpr operands binary_operand_to_operands(const ast::ast_expression_binary::operand_type operand, const bool use_key = false) noexcept
	{
		switch (operand)// NOLINT(clang-diagnostic-switch-enum)
		{
				using enum ast::ast_expression_binary::operand_type;
			case binary_plus: { return use_key ? operands::plus_key : operands::plus; }
			case binary_minus: { return use_key ? operands::minus_key : operands::minus; }
			case binary_multiply: { return use_key ? operands::multiply_key : operands::multiply; }
			case binary_divide: { return use_key ? operands::divide_key : operands::divide; }
			case binary_modulus: { return use_key ? operands::modulus_key : operands::modulus; }
			case binary_pow: { return use_key ? operands::pow_key : operands::pow; }
			default:
			{
				UNREACHABLE();
				gal_assert(false, "Unexpected binary operation!");
				return operands::nop;
			}
		}
	}

	[[nodiscard]] constexpr operands binary_operand_to_jump_operands(const ast::ast_expression_binary::operand_type operand, const bool use_not = false) noexcept
	{
		switch (operand)// NOLINT(clang-diagnostic-switch-enum)
		{
				using enum ast::ast_expression_binary::operand_type;
			case binary_equal: { return use_not ? operands::jump_if_not_equal : operands::jump_if_equal; }
			case binary_not_equal: { return use_not ? operands::jump_if_equal : operands::jump_if_not_equal; }
			case binary_less_than:
			case binary_greater_than: { return use_not ? operands::jump_if_not_less_than : operands::jump_if_less_than; }
			case binary_less_equal:
			case binary_greater_equal: { return use_not ? operands::jump_if_not_less_equal : operands::jump_if_less_equal; }
			default:
			{
				UNREACHABLE();
				gal_assert(false, "Unexpected binary operation!");
				return operands::nop;
			}
		}
	}

	class compiler
	{
	public:
		using stack_size_type = std::uint32_t;
		using register_size_type = stack_size_type;
		using label_type = bytecode_builder::label_type;
		using function_id_type = bytecode_builder::function_id_type;
		using register_type = bytecode_builder::register_type;
		using debug_pc_type = bytecode_builder::debug_pc_type;

		constexpr static register_size_type max_register_size = std::numeric_limits<register_type>::max();
		constexpr static register_size_type max_upvalue_size = 200;
		constexpr static register_size_type max_local_size = 200;

		struct constant_result
		{
			using invalid_type = void*;
			using null_type = bytecode_builder::constant::null_type;
			using boolean_type = bytecode_builder::constant::boolean_type;
			using number_type = bytecode_builder::constant::number_type;
			using string_type = bytecode_builder::string_ref_type;

			static_assert(not std::is_same_v<invalid_type, null_type>);

			using constant_type = std::variant<
				invalid_type,
				null_type,
				boolean_type,
				number_type,
				string_type>;

			constant_type data;

			[[nodiscard]] constexpr bool is_valid() const noexcept { return not std::holds_alternative<invalid_type>(data); }

			friend GAL_ASSERT_CONSTEXPR bool operator==(const constant_result& lhs, const constant_result& rhs) noexcept
			{
				gal_assert(lhs.is_valid() && rhs.is_valid());
				return lhs.data == rhs.data;
			}

			GAL_ASSERT_CONSTEXPR explicit operator bool() const noexcept
			{
				gal_assert(is_valid());
				return not std::holds_alternative<null_type>(data) && not(std::holds_alternative<boolean_type>(data) && std::get<boolean_type>(data) == false);
			}

			template<typename T>
				requires utils::is_any_type_of_v<T, null_type, boolean_type, number_type, string_type>
			[[nodiscard]] constexpr decltype(auto) get_if() noexcept { return std::get_if<T>(&data); }

			template<typename T>
				requires utils::is_any_type_of_v<T, null_type, boolean_type, number_type, string_type>
			[[nodiscard]] constexpr decltype(auto) get_if() const noexcept { return std::get_if<T>(&data); }

			template<typename Visitor>
			constexpr decltype(auto) visit(Visitor visitor) { return std::visit(visitor, data); }

			template<typename Visitor>
			[[nodiscard]] constexpr decltype(auto) visit(Visitor visitor) const { return std::visit(visitor, data); }

			constexpr static constant_result invalid_constant() noexcept { return {invalid_type{}}; }
		};

		struct scoped_register
		{
			compiler& self;
			register_size_type previous_top;

			constexpr explicit scoped_register(compiler& self)
				: self{self},
				  previous_top{self.register_top_} {}

			// This ctor is useful to forcefully adjust the stack frame in case we know that registers after a certain point are scratch and can be discarded
			GAL_ASSERT_CONSTEXPR scoped_register(compiler& self, const register_size_type top)
				: self{self},
				  previous_top{top}
			{
				gal_assert(top <= self.register_top_);
				self.register_top_ = top;
			}

			constexpr scoped_register(const scoped_register&) = delete;
			constexpr scoped_register& operator=(const scoped_register&) = delete;
			constexpr scoped_register(scoped_register&&) = delete;
			constexpr scoped_register& operator=(scoped_register&&) = delete;

			constexpr ~scoped_register() noexcept { self.register_top_ = previous_top; }
		};

		struct left_value
		{
			enum class value_type
			{
				local,
				upvalue,
				global,
				index_name,
				index_number,
				index_expression
			};

			value_type type;
			register_type reg;// register for local (local) or table (index*)
			register_type upvalue;
			register_type index; // register for index in Index_expression
			register_type number;// index-1 (0-255) in Index_number
			bytecode_builder::string_ref_type name;
			utils::location loc;
		};

		struct function_result
		{
			function_id_type id;
			std::vector<ast::ast_local*> upvalues;
		};

		struct local_result
		{
			register_type reg;
			bool allocated;
			bool captured;
			bool written;
			ast::ast_expression* init;
			debug_pc_type debug_pc;
			constant_result constant;
			ast::ast_expression_function* function;
		};

		struct global_result
		{
			bool writable;
			bool written;
		};

		struct builtin_method
		{
			using method_id_type = std::underlying_type_t<builtin_function>;
			constexpr static method_id_type invalid_method_id = static_cast<method_id_type>(-1);

			ast::ast_name object;
			ast::ast_name method;

			[[nodiscard]] constexpr bool empty() const noexcept { return object.empty() && method.empty(); }

			[[nodiscard]] constexpr bool is_global(const ast::ast_name name) const noexcept { return object.empty() && method == name; }

			[[nodiscard]] constexpr bool is_method(const ast::ast_name table, const ast::ast_name name) const noexcept { return object == table && method == name; }
		};

		struct loop_jump_result
		{
			enum class jump_type
			{
				jump_break,
				jump_continue
			};

			jump_type type;
			label_type label;
		};

		struct loop_result
		{
			label_type local_offset;
			ast::ast_expression* until_condition;
		};

	private:
		bytecode_builder& bytecode_;

		compile_options options_;

		utils::hash_map<ast::ast_expression_function*, function_result> functions_;
		utils::hash_map<ast::ast_local*, local_result> locals_;
		utils::hash_map<ast::ast_name, global_result> globals_;
		utils::hash_map<ast::ast_expression*, constant_result> constants_;
		utils::hash_map<ast::ast_expression_table*, std::pair<std::size_t, std::size_t>> predicted_table_size_;

		stack_size_type stack_size_;
		register_size_type register_top_;

		std::vector<ast::ast_local*> local_stack_;
		std::vector<ast::ast_local*> upvalues_;
		std::vector<loop_jump_result> loop_jumps_;
		std::vector<loop_result> loops_;

		bool use_get_function_environment_;
		bool use_set_function_environment_;

	public:
		class assignment_visitor final : public ast::ast_visitor
		{
		public:
			using field_type = std::pair<ast::ast_expression_table*, ast::ast_name>;

		private:
			struct field_type_hasher
			{
				[[nodiscard]] std::size_t operator()(const field_type& filed) const noexcept { return std::hash<field_type::first_type>{}(filed.first) ^ std::hash<field_type::second_type>{}(filed.second); }
			};

			utils::hash_map<ast::ast_local*, ast::ast_expression_table*> local_to_table_;
			utils::hash_set<field_type, field_type_hasher> fields_;
			compiler& self_;

		public:
			explicit assignment_visitor(compiler& self)
				: self_{self} {}

			void set_field(const ast::ast_expression* expression, const ast::ast_name index) { if (const auto* local = expression->as<ast::ast_expression_local>(); local) { if (const auto it = local_to_table_.find(local->get_local()); it != local_to_table_.end()) { if (fields_.emplace(it->second, index).second) { self_.predicted_table_size_[it->second].first += 1; } } } }

			void set_field(const ast::ast_expression* expression, const ast::ast_expression* index)
			{
				const auto* local = expression->as<ast::ast_expression_local>();
				const auto* number = index->as<ast::ast_expression_constant_number>();

				if (local && number)
				{
					if (const auto it = local_to_table_.find(local->get_local()); it != local_to_table_.end())
					{
						if (auto& array_size = self_.predicted_table_size_[it->second].second;
							static_cast<decltype(self_.predicted_table_size_[it->second].second)>(number->get()) == array_size + 1) { array_size += 1; }
					}
				}
			}

			void set(ast::ast_expression* var)
			{
				if (const auto* local = var->as<ast::ast_expression_local>(); local) { self_.locals_[local->get_local()].written = true; }
				else if (const auto* global = var->as<ast::ast_expression_global>(); global) { self_.globals_[global->get_name()].written = true; }
				else if (const auto* index_name = var->as<ast::ast_expression_index_name>(); index_name)
				{
					set_field(index_name->get_expression(), index_name->get_index());

					var->visit(*this);
				}
				else if (const auto* index_expression = var->as<ast::ast_expression_index_expression>(); index_expression)
				{
					set_field(index_expression->get_expression(), index_expression->get_index());

					var->visit(*this);
				}
				else
				{
					// we need to be able to track assignments in all expressions, including some crazy ones
					var->visit(*this);
				}
			}

			static ast::ast_expression_table* get_table_hint(ast::ast_expression* expression)
			{
				if (auto* table = expression->as<ast::ast_expression_table>(); table) { return table; }

				// set_meta_table(table literal, ...)
				// todo
				if (auto* call = expression->as<ast::ast_expression_call>();
					call && not call->has_self() && call->get_arg_size() == 2)
				{
					if (auto* function = call->get_function()->as<ast::ast_expression_global>();
						function && function->get_name() == "set_meta_table") { if (auto* table = call->get_arg(0)->as<ast::ast_expression_table>(); table) { return table; } }
				}

				return nullptr;
			}

			static const ast::ast_expression_table* get_table_hint(const ast::ast_expression* expression) { return get_table_hint(const_cast<ast::ast_expression*>(expression)); }

			bool visit(ast::ast_node& node) override
			{
				// track local -> table association so that we can update table size prediction in set_field
				if (auto* local = node.as<ast::ast_statement_local>(); local)
				{
					if (local->get_var_size() == 1 && local->get_value_size() == 1)
					{
						if (auto* table = get_table_hint(local->get_value(0));
							table && table->empty()) { local_to_table_[local->get_var(0)] = table; }
					}

					return true;
				}

				if (auto* assign = node.as<ast::ast_statement_assign>(); assign)
				{
					for (decltype(assign->get_var_size()) i = 0; i < assign->get_var_size(); ++i) { set(assign->get_var(i)); }

					for (decltype(assign->get_value_size()) i = 0; i < assign->get_value_size(); ++i) { assign->get_value(i)->visit(*this); }

					return false;
				}

				if (auto* compound_assign = node.as<ast::ast_statement_compound_assign>(); compound_assign)
				{
					set(compound_assign->get_var());
					compound_assign->get_value()->visit(*this);

					return false;
				}

				if (auto* function = node.as<ast::ast_statement_function>(); function)
				{
					set(function->get_name());
					function->get_function()->visit(*this);

					return false;
				}

				UNREACHABLE();
				gal_assert(false);
			}
		};

		class constant_visitor final : public ast::ast_visitor
		{
		private:
			compiler& self_;

		public:
			explicit constant_visitor(compiler& self)
				: self_{self} {}

			[[nodiscard]] constant_result analyze_unary(const ast::ast_expression_unary::operand_type operand, const constant_result& arg) const
			{
				switch (operand)
				{
						using enum ast::ast_expression_unary::operand_type;
					case unary_plus:
					{
						if (auto* number = arg.get_if<constant_result::number_type>(); number) { return {std::abs(*number)}; }
						return constant_result::invalid_constant();
					}
					case unary_minus:
					{
						if (auto* number = arg.get_if<constant_result::number_type>(); number) { return {-*number}; }
						return constant_result::invalid_constant();
					}
					case unary_not:
					{
						if (arg.is_valid()) { return {arg.operator bool()}; }
						return constant_result::invalid_constant();
					}
					default: // NOLINT(clang-diagnostic-covered-switch-default)
					{
						UNREACHABLE();
						gal_assert(false, "Unexpected unary operands!");
					}
				}
			}

			[[nodiscard]] constant_result analyze_binary(const ast::ast_expression_binary::operand_type operand, const constant_result& lhs, const constant_result& rhs) const
			{
				switch (operand)
				{
						using enum ast::ast_expression_binary::operand_type;
					case binary_plus:
					case binary_minus:
					case binary_multiply:
					case binary_divide:
					case binary_modulus:
					case binary_pow:
					case binary_less_than:
					case binary_less_equal:
					case binary_greater_than:
					case binary_greater_equal:
					{
						if (auto
									* lhs_number = lhs.get_if<constant_result::number_type>(),
									* rhs_number = rhs.get_if<constant_result::number_type>();
							lhs_number && rhs_number)
						{
							switch (operand)// NOLINT(clang-diagnostic-switch-enum)
							{
								case binary_plus: { return {*lhs_number + *rhs_number}; }
								case binary_minus: { return {*lhs_number - *rhs_number}; }
								case binary_multiply: { return {*lhs_number * *rhs_number}; }
								case binary_divide: { return {*lhs_number / *rhs_number}; }
								case binary_modulus: { return {*lhs_number - std::floor(*lhs_number / *rhs_number) * *rhs_number}; }
								case binary_pow: { return {std::pow(*lhs_number, *rhs_number)}; }
								case binary_less_than: { return {*lhs_number < *rhs_number}; }
								case binary_less_equal: { return {*lhs_number <= *rhs_number}; }
								case binary_greater_than: { return {*lhs_number > *rhs_number}; }
								case binary_greater_equal: { return {*lhs_number >= *rhs_number}; }
								default:
								{
									UNREACHABLE();
									gal_assert(false, "Impossible happened!");
								}
							}
						}
						break;
					}
					case binary_logical_and:
					{
						if (lhs.is_valid()) { return lhs.operator bool() ? rhs : lhs; }
						break;
					}
					case binary_logical_or:
					{
						if (lhs.is_valid()) { return lhs.operator bool() ? lhs : rhs; }
						break;
					}
					case binary_equal: { return {lhs == rhs}; }
					case binary_not_equal: { return {!(lhs == rhs)}; }
					default: // NOLINT(clang-diagnostic-covered-switch-default)
					{
						UNREACHABLE();
						gal_assert(false, "Unexpected binary operands!");
					}
				}

				return constant_result::invalid_constant();
			}

			[[nodiscard]] constant_result analyze(ast::ast_expression* node)
			{
				auto set_constant = [this, node](const constant_result result)
				{
					if (result.is_valid()) { self_.constants_[node] = result; }
					return result;
				};

				if (auto* expression_group = node->as<ast::ast_expression_group>(); expression_group) { return set_constant(analyze(expression_group->get_expression())); }
				if (node->is<ast::ast_expression_constant_null>()) { return set_constant({constant_result::null_type{}}); }
				if (const auto* cb = node->as<ast::ast_expression_constant_boolean>(); cb) { return set_constant({cb->get()}); }
				if (const auto* cn = node->as<ast::ast_expression_constant_number>(); cn) { return set_constant({cn->get()}); }
				if (const auto* cs = node->as<ast::ast_expression_constant_string>()) { return set_constant({cs->get()}); }
				if (const auto* local = node->as<ast::ast_expression_local>(); local)
				{
					if (const auto it = self_.locals_.find(local->get_local()); it != self_.locals_.end() && it->second.constant.is_valid())
					{
						gal_assert(not it->second.written);
						return set_constant(it->second.constant);
					}
					return constant_result::invalid_constant();
				}
				// nop ast_expression_global
				// nop ast_expression_varargs
				if (auto* call = node->as<ast::ast_expression_call>(); call)
				{
					(void)analyze(call->get_function());
					for (decltype(call->get_arg_size()) i = 0; i < call->get_arg_size(); ++i) { (void)analyze(call->get_arg(i)); }
					return constant_result::invalid_constant();
				}
				if (auto* index_name = node->as<ast::ast_expression_index_name>(); index_name)
				{
					(void)analyze(index_name->get_expression());
					return constant_result::invalid_constant();
				}
				if (auto* index_expression = node->as<ast::ast_expression_index_expression>(); index_expression)
				{
					(void)analyze(index_expression->get_expression());
					(void)analyze(index_expression->get_index());
					return constant_result::invalid_constant();
				}
				if (auto* function = node->as<ast::ast_expression_function>(); function)
				{
					// this is necessary to propagate constant information in all child functions
					function->get_body()->visit(*this);
					return constant_result::invalid_constant();
				}
				if (auto* table = node->as<ast::ast_expression_table>(); table)
				{
					for (decltype(table->get_item_size()) i = 0; i < table->get_item_size(); ++i)
					{
						auto [_, key, value] = table->get_item(i);
						if (key) { (void)analyze(key); }
						(void)analyze(value);
					}
					return constant_result::invalid_constant();
				}
				if (auto* unary = node->as<ast::ast_expression_unary>(); unary) { return set_constant(analyze_unary(unary->get_operand(), analyze(unary->get_expression()))); }
				if (auto* binary = node->as<ast::ast_expression_binary>(); binary) { return set_constant(analyze_binary(binary->get_operand(), analyze(binary->get_lhs_expression()), analyze(binary->get_rhs_expression()))); }
				if (auto* type_assertion = node->as<ast::ast_expression_type_assertion>(); type_assertion) { return analyze(type_assertion->get_expression()); }
				if (auto* if_else = node->as<ast::ast_expression_if_else>(); if_else)
				{
					// todo
					// if (const auto condition = analyze(if_else->get_condition());
					// 	condition.is_valid()) { return condition.operator bool() ? analyze(if_else->get_true_expression()) : analyze(if_else->get_false_expression()); }

					return constant_result::invalid_constant();
				}

				gal_assert(false, "Unknown expression type!");
				return constant_result::invalid_constant();
			}

			bool visit(ast::ast_node& node) override
			{
				// note: we short-circuit the visitor traversal through any expression trees by returning false
				// recursive traversal is happening inside analyze() which makes it easier to get the resulting value of the subexpression
				if (auto* expression = node.as_expression(); expression)
				{
					(void)analyze(expression);
					return false;
				}

				if (auto* local = node.as<ast::ast_statement_local>(); local)
				{
					// for values that match 1-1 we record the initializing expression for future analysis
					for (decltype(local->get_var_size()) i = 0; i < local->get_var_size() && i < local->get_value_size(); ++i) { self_.locals_[local->get_var(i)].init = local->get_value(i); }

					// all values that align wrt indexing are simple - we just match them 1-1
					for (decltype(local->get_var_size()) i = 0; i < local->get_var_size() && i < local->get_value_size(); ++i)
					{
						if (const auto arg = analyze(local->get_value(i));
							arg.is_valid())
						{
							// note: we rely on assignment_visitor to have been run before us
							if (auto& l = self_.locals_[local->get_var(i)]; not l.written) { l.constant = arg; }
						}
					}

					if (local->get_var_size() > local->get_value_size())
					{
						// if we have trailing variables, then depending on whether the last value is capable of returning multiple values
						// (aka call or varargs), we either don't know anything about these vars, or we know they're null
						auto* last = not local->empty_value() ? local->get_value(local->get_value_size() - 1) : nullptr;
						const bool multiple_return = last && (last->is<ast::ast_expression_call>() || last->is<ast::ast_expression_varargs>());

						for (auto i = local->get_value_size(); i < local->get_var_size(); ++i)
						{
							if (not multiple_return)
							{
								// note: we rely on assignment_visitor to have been run before us
								if (auto& l = self_.locals_[local->get_var(i)]; not l.written) { l.constant = {constant_result::null_type{}}; }
							}
						}
					}
					else
					{
						// we can have more values than variables
						// in this case we still need to analyze them to make sure we do constant propagation inside them
						for (auto i = local->get_var_size(); i < local->get_value_size(); ++i) { (void)analyze(local->get_value(i)); }
					}

					return false;
				}

				UNREACHABLE();
				gal_assert(false, "Not supported node type!");
				return false;
			}
		};

		class function_environment_visitor final : public ast::ast_visitor
		{
		private:
			bool& getter_used_;
			bool& setter_used_;

		public:
			constexpr function_environment_visitor(bool& getter_used, bool& setter_used)
				: getter_used_{getter_used},
				  setter_used_{setter_used} {}

			bool visit(ast::ast_node& node) override
			{
				if (auto* global = node.as<ast::ast_expression_global>(); global)
				{
					if (const auto name = global->get_name(); name == builtin_get_function_environment) { getter_used_ = true; }
					else if (name == builtin_set_function_environment) { setter_used_ = true; }

					return false;
				}

				UNREACHABLE();
				gal_assert(false, "Not supported node type!");
				return false;
			}
		};

		class function_visitor final : public ast::ast_visitor
		{
		public:
			using functions_type = std::vector<ast::ast_expression_function*>&;

		private:
			compiler& self_;
			functions_type functions_;

			constexpr function_visitor(compiler& self, functions_type functions)
				: self_{self},
				  functions_{functions} {}

			bool visit(ast::ast_node& node) override
			{
				if (auto* function = node.as<ast::ast_expression_function>(); function)
				{
					function->get_body()->visit(*this);

					// this makes sure all functions that are used when compiling this one have been already added to the vector
					functions_.push_back(function);

					return false;
				}

				if (auto* function = node.as<ast::ast_statement_function_local>(); function)
				{
					// record local->function association for some optimizations
					self_.locals_[function->get_name()].function = function->get_function();

					return true;
				}

				UNREACHABLE();
				gal_assert(false, "Not supported node type!");
				return false;
			}
		};

		class undefined_local_visitor final : public ast::ast_visitor
		{
		private:
			compiler& self_;
			ast::ast_local* undefined_;

		public:
			constexpr explicit undefined_local_visitor(compiler& self)
				: self_{self},
				  undefined_{nullptr} {}

			void check(ast::ast_local* local) { if (not self_.locals_[local].allocated && not undefined_) { undefined_ = local; } }

			bool visit(ast::ast_node& node) override
			{
				if (auto* local = node.as<ast::ast_expression_local>(); local)
				{
					if (not local->is_upvalue()) { check(local->get_local()); }

					return false;
				}

				if (auto* function = node.as<ast::ast_expression_function>(); function)
				{
					const auto it = self_.functions_.find(function);
					gal_assert(it != self_.functions_.end());

					for (auto* upvalue: it->second.upvalues)
					{
						gal_assert(upvalue->function_depth < function->get_function_depth());

						if (upvalue->function_depth == function->get_function_depth() - 1) { check(upvalue); }
					}

					return false;
				}

				UNREACHABLE();
				gal_assert(false, "Not supported node type!");
				return false;
			}

			[[nodiscard]] constexpr auto* get_undefined() noexcept { return undefined_; }

			[[nodiscard]] constexpr const auto* get_undefined() const noexcept { return undefined_; }
		};

		class const_upvalue_visitor final : public ast::ast_visitor
		{
		private:
			compiler& self_;
			std::vector<ast::ast_local*> upvalues_;

		public:
			explicit const_upvalue_visitor(compiler& self)
				: self_{self} {}

			bool visit(ast::ast_node& node) override
			{
				if (auto* local = node.as<ast::ast_expression_local>(); local)
				{
					if (local->is_upvalue() && self_.is_constant(local)) { upvalues_.push_back(local->get_local()); }

					return false;
				}

				if (auto* function = node.as<ast::ast_expression_function>(); function)
				{
					// short-circuits the traversal to make it faster
					return false;
				}

				UNREACHABLE();
				gal_assert(false, "Not supported node type!");
				return false;
			}

			[[nodiscard]] std::vector<ast::ast_local*>& get_upvalues() noexcept { return upvalues_; }

			[[nodiscard]] const std::vector<ast::ast_local*>& get_upvalues() const noexcept { return upvalues_; }
		};

		compiler(bytecode_builder& bytecode, compile_options options)
			: bytecode_{bytecode},
			  options_{options},
			  stack_size_{0},
			  register_top_{0},
			  use_get_function_environment_{false},
			  use_set_function_environment_{false} {}

		[[nodiscard]] register_type get_local(const ast::ast_local* local) const
		{
			const auto it = locals_.find(local);

			gal_assert(it != locals_.end());
			gal_assert(it->second.allocated);

			return it->second.reg;
		}

		register_type get_upvalue(ast::ast_local* local)
		{
			if (const auto it = std::ranges::find(upvalues_, local); it != upvalues_.end()) { return static_cast<register_type>(std::distance(it, upvalues_.begin())); }

			if (upvalues_.size() >= max_upvalue_size) { throw compile_error{local->loc, std_format::format("Out of upvalue registers when trying to allocate {}: exceeded limit {}", local->name, max_upvalue_size)}; }

			// mark local as captured so that close_locals emits CLOSE_UPVALUES accordingly
			locals_[local].captured = true;
			upvalues_.push_back(local);

			return static_cast<register_type>(upvalues_.size() - 1);
		}

		constexpr void emit_load_key(const operand_abc_underlying_type target, const operand_aux_underlying_type id) const
		{
			if (std::cmp_less_equal(id, std::numeric_limits<operand_d_underlying_type>::max())) { bytecode_.emit_operand_ad(operands::load_key, target, static_cast<operand_d_underlying_type>(id)); }
			else
			{
				bytecode_.emit_operand_ad(operands::load_key_extra, target, 0);
				bytecode_.emit_operand_aux(id);
			}
		}

		function_id_type compile_function(ast::ast_expression_function* function)
		{
			// todo: timer?

			gal_assert(not functions_.contains(function));
			gal_assert(register_top_ == 0 && stack_size_ == 0 && local_stack_.empty() && upvalues_.empty());

			scoped_register scoped{*this};

			const bool has_self = function->has_self();
			const bool is_vararg = function->is_vararg();

			const auto function_args = static_cast<operand_abc_underlying_type>(function->get_arg_size() + has_self);

			auto id = bytecode_.begin_function(function_args, is_vararg);

			set_debug_line(function);

			if (is_vararg) { bytecode_.emit_operand_abc(operands::prepare_varargs, function_args, 0, 0); }

			const auto args = new_registers(function, function_args);

			if (has_self) { push_local(function->get_self(), args); }

			for (decltype(function->get_arg_size()) i = 0; i < function->get_arg_size(); ++i) { push_local(function->get_arg(i), static_cast<register_type>(args + has_self + 1)); }

			auto* body = function->get_body();

			for (decltype(body->get_body_size()) i = 0; i < body->get_body_size(); ++i) { compile_statement(body->get_body(i)); }

			// valid function bytecode must always end with RETURN
			// we elide this if we're guaranteed to hit a RETURN statement regardless of the control flow
			if (not body->all_control_path_has_return())
			{
				set_debug_line<true>(body);
				close_locals(0);

				bytecode_.emit_operand_abc(operands::call_return, 0, 1, 0);
			}

			// constant folding may remove some upvalue refs from bytecode, so this puts them back
			if (options_.optimization_level >= 1 && options_.debug_level >= 2) { gather_cons_upvalue(function); }

			if (options_.debug_level >= 1 && not function->get_debug_name().empty()) { bytecode_.set_debug_function_name(function->get_debug_name()); }

			if (options_.debug_level >= 2 && not upvalues_.empty()) { for (const auto* upvalue: upvalues_) { bytecode_.push_debug_upvalue(upvalue->get_name()); } }

			if (options_.optimization_level >= 1) { bytecode_.fold_jumps(); }

			pop_locals(0);

			bytecode_.end_function(
					static_cast<operand_abc_underlying_type>(stack_size_),
					static_cast<operand_abc_underlying_type>(upvalues_.size()));

			stack_size_ = 0;

			functions_[function] = {
					.id = id,
					.upvalues = std::move(upvalues_)};

			return id;
		}

		/**
		 * @note this does not just clobber target (assuming it's temp), but also clobbers *all* allocated registers >= target!
		 *
		 * this is important to be able to support "multiple return" semantics due to call frame structure
		 */
		bool compile_expression_temp_multiple_return(ast::ast_expression* node, const register_type target)
		{
			if (auto* call = node->as<ast::ast_expression_call>(); call)
			{
				// We temporarily swap out register_top_ to have register_top_ work correctly...
				// This is a crude hack but it's necessary for correctness :(
				scoped_register scoped{*this, target};
				compile_expression_call(call, target, 0, true, true);
				return true;
			}

			if (auto* vararg = node->as<ast::ast_expression_varargs>(); vararg)
			{
				// We temporarily swap out register_top_ to have register_top_ work correctly...
				// This is a crude hack but it's necessary for correctness :(
				scoped_register scoped{*this, target};
				compile_expression_varargs(vararg, target, 0, true);
				return true;
			}

			compile_expression_temp(node, target);
			return false;
		}

		/**
		 * @note this does not just clobber target (assuming it's temp), but also clobbers *all* allocated registers >= target!
		 *
		 * this is important to be able to emit code that takes fewer registers and runs faster
		 */
		void compile_expression_temp_top(ast::ast_expression* node, const register_type target)
		{
			// We temporarily swap out register_top_ to have register_top_ work correctly...
			// This is a crude hack but it's necessary for performance :(
			// It makes sure that nested call expressions can use target_top optimization and do not need to have too many registers
			scoped_register scoped{*this, static_cast<register_size_type>(target + 1)};
			compile_expression_temp(node, target);
		}

		void compile_expression_varargs(const ast::ast_expression_varargs* varargs, const register_type target, const register_size_type target_count, const bool multiple_return = false) const
		{
			gal_assert(not multiple_return || target + target_count == register_top_);

			// normally compile_expression sets up line info, but compile_expression_call can be called directly
			set_debug_line(varargs);

			bytecode_.emit_operand_abc(operands::load_varargs, target, multiple_return ? 0 : static_cast<operand_abc_underlying_type>(target_count + 1), 0);
		}

		void compile_expression_call(ast::ast_expression_call* call, const register_type target, const register_size_type target_count, const bool target_top = false, const bool multiple_return = false)
		{
			gal_assert(not target_top || target + target_count == register_top_);

			// normally compile_expression sets up line info, but compile_expression_call can be called directly
			set_debug_line(call);

			scoped_register scoped{*this};

			const auto needed = std::ranges::max(static_cast<register_size_type>(1 + call->has_self() + call->get_arg_size()), target_count);

			// Optimization: if target points to the top of the stack, we can start the call at previous_top - 1 and won't need MOVE at the end
			const auto registers = static_cast<register_type>(target_top ? new_registers(call, needed - target_count) - target_count : new_registers(call, needed));

			register_type self_reg = 0;

			builtin_method::method_id_type method_id = builtin_method::invalid_method_id;

			if (options_.optimization_level >= 1) { method_id = get_builtin_method_id(get_builtin(call->get_function())); }

			gal_assert(method_id >= builtin_method::invalid_method_id);

			if (call->has_self())
			{
				auto* index_name = call->get_function()->as<ast::ast_expression_index_name>();
				gal_assert(index_name);

				// Optimization: use local register directly in NAMED_CALL if possible
				if (auto* expression = index_name->get_expression(); is_expression_local_register(expression)) { self_reg = get_local(expression->as<ast::ast_expression_local>()->get_local()); }
				else
				{
					// Note: to be able to compile very deeply nested self call chains (object@method1()@method2()@...), we need to be able to do this in
					// finite stack space NAMED_CALL will happily move object from registers to registers+1 but we need to compute it into registers so that
					// compile_expression_temp_top does not increase stack usage for every recursive call
					self_reg = registers;

					compile_expression_temp_top(expression, self_reg);
				}
			}
			else if (method_id == builtin_method::invalid_method_id) { compile_expression_temp_top(call->get_function(), registers); }

			// Note: if the last argument is expression_vararg or expression_call, we need to route that directly to the called function preserving the # of args
			bool multiple_call = false;
			bool skip_args = false;

			if (not call->has_self() && method_id != builtin_method::invalid_method_id && call->get_arg_size() >= 1 && call->get_arg_size() <= 2)
			{
				const auto* last = call->get_arg(call->get_arg_size() - 1);
				skip_args = not last->is<ast::ast_expression_call>() && not last->is<ast::ast_expression_varargs>();
			}

			if (not skip_args)
			{
				for (decltype(call->get_arg_size()) i = 0; i < call->get_arg_size(); ++i)
				{
					const auto t = static_cast<register_type>(registers + 1 + call->has_self() + i);

					if (i + 1 == call->get_arg_size()) { multiple_call = compile_expression_temp_multiple_return(call->get_arg(i), t); }
					else { compile_expression_temp_top(call->get_arg(i), t); }
				}
			}

			set_debug_line<true>(call->get_function());

			if (call->has_self())
			{
				auto* index_name = call->get_function()->as<ast::ast_expression_index_name>();
				gal_assert(index_name);

				set_debug_line(index_name->get_index_location());

				const auto name = index_name->get_index();
				const auto id = bytecode_.add_constant_string(name);
				if (id == bytecode_builder::constant_too_much_index)
				{
					throw compile_error{index_name->get_location(),
					                    std_format::format("Exceeded constant limit: {}; simplify the code to compile", bytecode_builder::max_constant_size)};
				}
				bytecode_.emit_operand_abc(operands::named_call, registers, self_reg, static_cast<operand_abc_underlying_type>(utils::short_string_hash(name)));
				bytecode_.emit_operand_aux(id);
			}
			else if (method_id != builtin_method::invalid_method_id)
			{
				label_type fastcall_label;

				if (skip_args)
				{
					auto operand = call->get_arg_size() == 1 ? operands::fastcall_1 : operands::fastcall_2;

					register_type args[2]{};
					for (decltype(call->get_arg_size()) i = 0; i < call->get_arg_size(); ++i)
					{
						if (i != 0)
						{
							if (const auto id = get_constant_index(call->get_arg(i));
								id != bytecode_builder::constant_too_much_index)
							{
								operand = operands::fastcall_2_key;
								args[i] = static_cast<operand_abc_underlying_type>(id);
								break;
							}
						}

						if (is_expression_local_register(call->get_arg(i))) { args[i] = get_local(call->get_arg(i)->as<ast::ast_expression_local>()->get_local()); }
						else
						{
							args[i] = static_cast<operand_abc_underlying_type>(registers + 1 + i);
							compile_expression_temp_top(call->get_arg(i), args[i]);
						}
					}

					fastcall_label = bytecode_.emit_label();
					bytecode_.emit_operand_abc(operand, method_id, args[0], 0);
					if (operand != operands::fastcall_1) { bytecode_.emit_operand_aux(args[1]); }

					// Set up a traditional stack for the subsequent CALL.
					// Note, as with other instructions that immediately follow FASTCALL, these are normally not executed and are used as a fallback for
					// these FASTCALL variants.
					for (decltype(call->get_arg_size()) i = 0; i < call->get_arg_size(); ++i)
					{
						const auto t = static_cast<register_type>(registers + 1 + i);

						if (i != 0 && operand == operands::fastcall_2_key)
						{
							emit_load_key(t, args[i]);
							break;
						}

						if (args[i] != t) { bytecode_.emit_operand_abc(operands::move, t, args[i], 0); }
					}
				}
				else
				{
					fastcall_label = bytecode_.emit_label();
					bytecode_.emit_operand_abc(operands::fastcall, method_id, 0, 0);
				}

				// note, these instructions are normally not executed and are used as a fallback for FASTCALL
				// we can't use temp_top variant here because we need to make sure the arguments we already computed aren't overwritten
				compile_expression_temp(call->get_function(), registers);

				// FASTCALL will skip over the instructions needed to compute function and jump over CALL which must immediately follow the instruction
				// sequence after FASTCALL
				if (const auto call_label = bytecode_.emit_label();
					not bytecode_.patch_skip_c(fastcall_label, call_label))
				{
					throw compile_error{call->get_function()->get_location(),
					                    std_format::format("Exceeded jump distance limit: {}; simplify the code to compile", bytecode_builder::max_jump_distance)};
				}
			}

			bytecode_.emit_operand_abc(
					operands::call,
					registers,
					multiple_call ? 0 : static_cast<operand_abc_underlying_type>(1 + call->has_self() + call->get_arg_size()),
					multiple_return ? 0 : static_cast<operand_abc_underlying_type>(target_count + 1));

			// if we didn't output results directly to target, we need to move them
			if (not target_top)
			{
				for (register_size_type i = 0; i < target_count; ++i)
				{
					bytecode_.emit_operand_abc(
							operands::move,
							static_cast<operand_abc_underlying_type>(target + i),
							static_cast<operand_abc_underlying_type>(registers + i),
							0);
				}
			}
		}

		bool should_share_closure(const ast::ast_expression_function* function)
		{
			const auto it = functions_.find(function);

			if (it == functions_.end()) { return false; }

			for (const auto* upvalue: it->second.upvalues)
			{
				const auto local = locals_.find(upvalue);
				gal_assert(local != locals_.end());

				if (local->second.written) { return false; }

				// it's technically safe to share closures whenever all upvalues are immutable
				// this is because of a runtime equality check in COPY_CLOSURE.
				// however, this results in frequent de-optimization and increases the set of reachable objects, making some temporary objects permanent
				// instead we apply a heuristic: we share closures if they refer to top-level upvalues, or closures that refer to top-level upvalues
				// this will only de-optimize (outside of function environment changes) if top level code is executed twice with different results.
				if (upvalue->function_depth != 0 || upvalue->loop_depth != 0)
				{
					if (not local->second.function) { return false; }

					if (local->second.function != function && not should_share_closure(local->second.function)) { return false; }
				}
			}

			return true;
		}

		void compile_expression_function(const ast::ast_expression_function* function, const register_type target)
		{
			const auto it = functions_.find(function);
			gal_assert(it != functions_.end());

			// when the closure has upvalues we'll use this to create the closure at runtime
			// when the closure has no upvalues, we use constant closures that technically do not rely on the child function list
			// however, it's still important to add the child function because debugger relies on the function hierarchy when setting breakpoints
			const auto child_id = bytecode_.add_child_function(it->second.id);
			if (child_id == bytecode_builder::constant_too_much_index)
			{
				throw compile_error{function->get_location(),
				                    std_format::format("Exceeded closure limit: {}; simplify the code to compile", bytecode_builder::max_constant_size)};
			}

			bool shared = false;

			// Optimization: when closure has no upvalues, or upvalues are safe to share, instead of allocating it every time we can share closure
			// objects (this breaks assumptions about function identity which can lead to set_function_environment not working as expected,
			// so we disable this when it is used)
			if (options_.optimization_level >= 1 && should_share_closure(function) && not use_set_function_environment_)
			{
				if (const auto id = bytecode_.add_constant_closure(it->second.id);
					id != bytecode_builder::constant_too_much_index && id <= std::numeric_limits<operand_d_underlying_type>::max())
				{
					bytecode_.emit_operand_ad(operands::copy_closure, target, static_cast<operand_d_underlying_type>(id));
					shared = true;
				}
			}

			if (not shared) { bytecode_.emit_operand_ad(operands::new_closure, target, static_cast<operand_d_underlying_type>(child_id)); }

			for (auto* upvalue: it->second.upvalues)
			{
				gal_assert(upvalue->function_depth < function->get_function_depth());

				const auto local = locals_.find(upvalue);
				gal_assert(local != locals_.end());

				const bool immutable = not local->second.written;

				if (upvalue->function_depth == function->get_function_depth() - 1)
				{
					// get local variable
					bytecode_.emit_operand_abc(
							operands::capture,
							static_cast<operand_abc_underlying_type>(immutable ? capture_type::value : capture_type::reference),
							get_local(upvalue),
							0);
				}
				else
				{
					// get upvalue from parent frame
					// note: this will add upvalue to the current upvalue list if necessary
					bytecode_.emit_operand_abc(
							operands::capture,
							static_cast<operand_abc_underlying_type>(capture_type::upvalue),
							get_upvalue(upvalue),
							0);
				}
			}
		}

	private:
		[[nodiscard]] auto get_constant(const ast::ast_expression* node) { return constants_.find(node); }

		[[nodiscard]] auto get_constant(const ast::ast_expression* node) const { return constants_.find(node); }

		[[nodiscard]] auto get_valid_constant(const ast::ast_expression* node)
		{
			if (auto it = get_constant(node); it != constants_.end() && it->second.is_valid()) { return it; }
			return constants_.end();
		}

		[[nodiscard]] auto get_valid_constant(const ast::ast_expression* node) const
		{
			if (const auto it = get_constant(node); it != constants_.end() && it->second.is_valid()) { return it; }
			return constants_.end();
		}

	public:
		[[nodiscard]] bool is_constant(const ast::ast_expression* node) const { return get_valid_constant(node) != constants_.end(); }

		[[nodiscard]] bool is_constant_true(const ast::ast_expression* node) const
		{
			const auto it = get_valid_constant(node);
			return it != constants_.end() && it->second.operator bool();
		}

		[[nodiscard]] bool is_constant_false(const ast::ast_expression* node) const
		{
			const auto it = get_valid_constant(node);
			return it != constants_.end() && not it->second.operator bool();
		}

		bytecode_builder::signed_index_type get_constant_number(const ast::ast_expression* node) const
		{
			const auto it = get_valid_constant(node);
			if (it != constants_.end())
			{
				if (auto* number = it->second.get_if<constant_result::number_type>(); number)
				{
					const auto id = bytecode_.add_constant_number(*number);
					gal_assert(id >= bytecode_builder::constant_too_much_index);
					if (id == bytecode_builder::constant_too_much_index)
					{
						throw compile_error{
								node->get_location(),
								std_format::format("Exceeded constant limit: {}; simplify the code to compile", bytecode_builder::max_constant_size)};
					}

					return id;
				}
			}

			return bytecode_builder::constant_too_much_index;
		}

		bytecode_builder::signed_index_type get_constant_index(const ast::ast_expression* node) const
		{
			const auto it = get_valid_constant(node);

			if (it == constants_.end()) { return bytecode_builder::constant_too_much_index; }

			const auto id = it->second.visit(
					[this]<typename T>(T&& data)
					{
						using type = T;
						if constexpr (std::is_same_v<T, constant_result::null_type>) { return bytecode_.add_constant_null(); }
						else if constexpr (std::is_same_v<T, constant_result::boolean_type>) { return bytecode_.add_constant_boolean(data); }
						else if constexpr (std::is_same_v<T, constant_result::number_type>) { return bytecode_.add_constant_number(data); }
						else if constexpr (std::is_same_v<T, constant_result::string_type>) { return bytecode_.add_constant_string(data); }
						else
						{
							UNREACHABLE();
							gal_assert(false, "Unexpected constant type!");
							return bytecode_builder::constant_too_much_index;
						}
					});

			gal_assert(id >= bytecode_builder::constant_too_much_index);
			if (id == bytecode_builder::constant_too_much_index)
			{
				throw compile_error{
						node->get_location(),
						std_format::format("Exceeded constant limit: {}; simplify the code to compile", bytecode_builder::max_constant_size)};
			}

			return id;
		}

		[[nodiscard]] label_type compile_compare_jump(ast::ast_expression_binary* expression_binary, bool use_not = false)
		{
			scoped_register scoped{*this};

			auto operand = binary_operand_to_jump_operands(expression_binary->get_operand(), use_not);

			const bool equal = utils::is_any_enum_of(operand, operands::jump_if_equal, operands::jump_if_not_equal);

			bool operand_is_constant = is_constant(expression_binary->get_rhs_expression());
			if (equal && not operand_is_constant)
			{
				operand_is_constant = is_constant(expression_binary->get_lhs_expression());
				if (operand_is_constant) { expression_binary->swap_lhs_and_rhs(); }
			}

			const auto lhs = compile_expression_auto(expression_binary->get_lhs_expression());
			bytecode_builder::signed_index_type rhs;

			if (equal && operand_is_constant)
			{
				if (operand == operands::jump_if_equal) { operand = operands::jump_if_equal_key; }
				else if (operand == operands::jump_if_not_equal) { operand = operands::jump_if_not_equal_key; }

				rhs = get_constant_index(expression_binary->get_rhs_expression());
				gal_assert(rhs != bytecode_builder::constant_too_much_index);
			}
			else { rhs = compile_expression_auto(expression_binary->get_rhs_expression()); }

			const auto jump_label = bytecode_.emit_label();

			if (utils::is_any_enum_of(expression_binary->get_operand(), ast::ast_expression_binary::operand_type::binary_greater_than, ast::ast_expression_binary::operand_type::binary_greater_equal))
			{
				bytecode_.emit_operand_ad(operand, static_cast<operand_abc_underlying_type>(rhs), 0);
				bytecode_.emit_operand_aux(lhs);
			}
			else
			{
				bytecode_.emit_operand_ad(operand, lhs, 0);
				bytecode_.emit_operand_aux(rhs);
			}

			return jump_label;
		}

		// compile expression to target temp register
		// if the expression (or not expression if only_truth is false) is truth, jump via skip_jump
		// if the expression (or not expression if only_truth is false) is falsy, fall through (target isn't guaranteed to be updated in this case)
		// if target is omitted, then the jump behavior is the same - skip_jump or fallthrough depending on the truthfulness of the expression
		void compile_condition_value(ast::ast_expression* node, const register_type* target, const std::vector<label_type>& skip_jump, const bool only_truth);

		// checks if compiling the expression as a condition value generates code that's faster than using compile_expression
		[[nodiscard]] bool is_condition_fast(ast::ast_expression* node);

		void compile_expression_logical(ast::ast_expression_binary* expression, register_type target, bool temp_target);

		void compile_expression_unary(ast::ast_expression_unary* expression, register_type target);

		void compile_expression_binary(ast::ast_expression_binary* expression, register_type target, bool temp_target);

		void compile_expression_if_else(ast::ast_expression_if_else* expression, register_type target, bool temp_target);

		void compile_expression_table(ast::ast_expression_table* expression, register_type target, bool temp_target);

		[[nodiscard]] bool importable(const ast::ast_expression_global* expression)
		{
			const auto it = globals_.find(expression->get_name());

			return options_.optimization_level >= 1 && (it == globals_.end() || not it->second.written);
		}

		[[nodiscard]] bool importable_chain(const ast::ast_expression_global* expression)
		{
			const auto it = globals_.find(expression->get_name());

			return options_.optimization_level >= 1 && (it == globals_.end() || (not it->second.written && not it->second.writable));
		}

		void compile_expression_index_name(ast::ast_expression_index_name* expression, register_type target);

		void compile_expression_index_expression(ast::ast_expression_index_expression* expression, register_type target);

		void compile_expression_global(ast::ast_expression_global* expression, register_type target);

		void compile_expression_constant(ast::ast_expression* node, const constant_result& c, register_type target);

		void compile_expression(ast::ast_expression* node, register_type target, bool temp_target = false);

		void compile_expression_temp(ast::ast_expression* node, register_type target) { compile_expression(node, target, true); }

		register_type compile_expression_auto(ast::ast_expression* node);

		// initializes target..target+target_count-1 range using expressions from the list
		// if list has fewer expressions, and last expression is a call, we assume the call returns the rest of the values
		// if list has fewer expressions, and last expression isn't a call, we fill the rest with null
		// assumes target register range can be clobbered and is at the top of the register space
		void compile_expression_list_top(ast::ast_array<ast::ast_expression*> list, register_type target, register_size_type target_count);

		left_value compile_lvalue(ast::ast_expression* node, scoped_register& scoped);

		void compile_lvalue_usage(const left_value& lv, register_type reg, bool set);

		void compile_assignment(const left_value& lv, register_type source) { compile_lvalue_usage(lv, source, true); }

		[[nodiscard]] bool is_expression_local_register(ast::ast_expression* expression) const
		{
			if (const auto* local = expression->as<ast::ast_expression_local>(); not local || local->is_upvalue()) { return false; }
			else
			{
				const auto it = locals_.find(local->get_local());
				gal_assert(it != locals_.end());

				return it->second.allocated;
			}
		}

		void compile_statement_if(ast::ast_statement_if* statement_if);

		void compile_statement_while(ast::ast_statement_while* statement_while);

		void compile_statement_repeat(ast::ast_statement_repeat* statement_repeat);

		void compile_statement_return(ast::ast_statement_return* statement_return);

		[[nodiscard]] bool are_locals_redundant(const ast::ast_statement_local* statement_local) const
		{
			// Extract expressions may have side effects
			if (statement_local->get_value_size() > statement_local->get_var_size()) { return false; }

			for (decltype(statement_local->get_var_size()) i = 0; i < statement_local->get_var_size(); ++i)
			{
				const auto it = locals_.find(statement_local->get_var(i));

				if (it == locals_.end() || not it->second.constant.is_valid()) { return false; }
			}

			return true;
		}

		void compile_statement_local(ast::ast_statement_local* statement_local);

		void compile_statement_for(ast::ast_statement_for* statement_for);

		void compile_statement_for_in(ast::ast_statement_for_in* statement_for_in);

		void resolve_assignment_conflicts(ast::ast_statement* node, std::vector<left_value>& vars);

		void compile_statement_assign(ast::ast_statement_assign* statement_assign);

		void compile_statement_compound_assgin(ast::ast_statement_compound_assign* statement_compound_assign);

		void compile_statement_function(ast::ast_statement_function* statement_function);

		void compile_statement(ast::ast_statement* node)
		{
			set_debug_line(node);

			if (options_.coverage_level >= 1 && node->need_coverage()) { bytecode_.emit_operand_abc(operands::coverage, 0, 0, 0); }

			if (auto* block = node->as<ast::ast_statement_block>(); block)
			{
				scoped_register scoped{*this};

				const auto previous_local_size = local_stack_.size();

				for (decltype(block->get_body_size()) i = 0; i < block->get_body_size(); ++i) { compile_statement(block->get_body(i)); }

				close_locals(previous_local_size);
				pop_locals(previous_local_size);
				return;
			}

			if (auto* i = node->as<ast::ast_statement_if>(); i)
			{
				compile_statement_if(i);
				return;
			}

			if (auto* w = node->as<ast::ast_statement_while>(); w)
			{
				compile_statement_while(w);
				return;
			}

			if (auto* r = node->as<ast::ast_statement_repeat>(); r)
			{
				compile_statement_repeat(r);
				return;
			}

			if (node->is<ast::ast_statement_break>())
			{
				gal_assert(not loops_.empty());

				// before exiting out of the loop, we need to close all local variables that were captured in closures since loop start
				// normally they are closed by the enclosing blocks, including the loop block, but we're skipping that here
				close_locals(loops_.back().local_offset);

				const auto label = bytecode_.emit_label();

				bytecode_.emit_operand_ad(operands::jump, 0, 0);

				loop_jumps_.emplace_back(loop_jump_result::jump_type::jump_break, label);
				return;
			}

			if (auto* c = node->as<ast::ast_statement_continue>(); c)
			{
				gal_assert(not loops_.empty());

				if (auto* condition = loops_.back().until_condition; condition) { validate_continue_until(c, condition); }

				// before continuing, we need to close all local variables that were captured in closures since loop start
				// normally they are closed by the enclosing blocks, including the loop block, but we're skipping that here
				close_locals(loops_.back().local_offset);

				const auto label = bytecode_.emit_label();

				bytecode_.emit_operand_ad(operands::jump, 0, 0);

				loop_jumps_.emplace_back(loop_jump_result::jump_type::jump_continue, label);
				return;
			}

			if (auto* r = node->as<ast::ast_statement_return>(); r)
			{
				compile_statement_return(r);
				return;
			}

			if (auto* e = node->as<ast::ast_statement_expression>(); e)
			{
				// Optimization: since we don't need to read anything from the stack, we can compile the call to not return anything which saves register moves
				if (auto* call = e->as<ast::ast_expression_call>(); call) { compile_expression_call(call, static_cast<register_type>(register_top_), 0); }
				else
				{
					scoped_register scoped{*this};
					compile_expression_auto(e->get_expression());
				}
				return;
			}

			if (auto* l = node->as<ast::ast_statement_local>(); l)
			{
				compile_statement_local(l);
				return;
			}

			if (auto* f = node->as<ast::ast_statement_for>(); f)
			{
				compile_statement_for(f);
				return;
			}

			if (auto* fi = node->as<ast::ast_statement_for_in>(); fi)
			{
				compile_statement_for_in(fi);
				return;
			}

			if (auto* a = node->as<ast::ast_statement_assign>(); a)
			{
				compile_statement_assign(a);
				return;
			}

			if (auto* ca = node->as<ast::ast_statement_compound_assign>(); ca)
			{
				compile_statement_compound_assgin(ca);
				return;
			}

			if (auto* f = node->as<ast::ast_statement_function>(); f)
			{
				compile_statement_function(f);
				return;
			}

			if (auto* fl = node->as<ast::ast_statement_function_local>(); fl)
			{
				const auto var = new_registers(fl, 1);

				push_local(fl->get_name(), var);
				compile_expression_function(fl->get_function(), var);

				// we *have* to push_local before we compile the function, since the function may refer to the local as an upvalue
				// however, this means the debug_pc for the local is at an instruction where the local value hasn't been computed yet
				// to fix this we just move the debug_pc after the local value is established
				locals_[fl->get_name()].debug_pc = bytecode_.get_debug_pc();
				return;
			}

			if (node->is<ast::ast_statement_type_alias>())
			{
				// do nothing
				return;
			}

			UNREACHABLE();
			gal_assert(false, "Unknown statement type!");
		}

		void validate_continue_until(const ast::ast_statement* continue_statement, ast::ast_expression* condition)
		{
			undefined_local_visitor visitor{*this};
			condition->visit(visitor);

			if (const auto* undefined = visitor.get_undefined(); undefined)
			{
				throw compile_error{
						condition->get_location(),
						std_format::format(
								"Local {} used in the repeat..until condition is undefined because continue statement on line {} jumps over it",
								undefined->get_name(),
								continue_statement->get_location().begin.line + 1
								)};
			}
		}

		void gather_cons_upvalue(ast::ast_expression_function* function)
		{
			const_upvalue_visitor visitor{*this};
			function->get_body()->visit(visitor);

			for (auto* local: visitor.get_upvalues()) { get_upvalue(local); }
		}

		void push_local(ast::ast_local* local, const register_type reg)
		{
			if (local_stack_.size() > max_local_size) { throw compile_error{local->loc, std_format::format("Out of local registers when trying to allocate {}({} already exist): exceeded limit {}", local->name, local_stack_.size(), max_local_size)}; }

			local_stack_.push_back(local);

			auto& l = locals_[local];

			gal_assert(not l.allocated);

			l.reg = reg;
			l.allocated = true;
			l.debug_pc = bytecode_.get_debug_pc();
		}

		[[nodiscard]] bool are_locals_captured(decltype(local_stack_.size()) begin) const
		{
			gal_assert(begin <= local_stack_.size());

			for (; begin < local_stack_.size(); ++begin)
			{
				const auto it = locals_.find(local_stack_[begin]);
				gal_assert(it != locals_.end());

				if (it->second.captured && it->second.written) { return true; }
			}

			return false;
		}

		void close_locals(decltype(local_stack_.size()) begin)
		{
			gal_assert(begin <= local_stack_.size());

			bool captured = false;
			auto capture_reg = std::numeric_limits<register_type>::max();

			for (; begin < local_stack_.size(); ++begin)
			{
				const auto it = locals_.find(local_stack_[begin]);
				gal_assert(it != locals_.end());

				if (it->second.captured && it->second.written)
				{
					captured = true;
					capture_reg = std::ranges::min(capture_reg, it->second.reg);
				}
			}

			if (captured) { bytecode_.emit_operand_abc(operands::close_upvalues, capture_reg, 0, 0); }
		}

		void pop_locals(const decltype(local_stack_.size()) begin)
		{
			gal_assert(begin <= local_stack_.size());

			for (auto i = begin; i < local_stack_.size(); ++i)
			{
				auto it = locals_.find(local_stack_[i]);
				gal_assert(it != locals_.end());
				gal_assert(it->second.allocated);

				it->second.allocated = false;

				if (options_.debug_level >= 2) { bytecode_.push_debug_local(local_stack_[i]->name, it->second.reg, it->second.debug_pc, bytecode_.get_debug_pc()); }
			}

			local_stack_.erase(std::next(local_stack_.begin(), static_cast<decltype(local_stack_)::difference_type>(begin)));
		}

		void patch_jump(const ast::ast_node* node, const label_type jump, const label_type target) const
		{
			if (not bytecode_.patch_jump_d(jump, target))
			{
				throw compile_error{
						node->get_location(),
						std_format::format("Exceeded jump distance limit: {}; simplify the code to compile", bytecode_builder::max_jump_distance)};
			}
		}

		void patch_jumps(const ast::ast_node* node, const std::vector<label_type>& jumps, label_type target) const { for (const auto jump: jumps) { patch_jump(node, jump, target); } }

		void patch_loop_jumps(const ast::ast_node* node, label_type previous_jump, const label_type end_label, const label_type continue_label)
		{
			gal_assert(previous_jump <= loop_jumps_.size());

			for (; previous_jump < loop_jumps_.size(); ++previous_jump)
			{
				switch (const auto [type, label] = loop_jumps_[previous_jump];
					type)
				{
						using enum loop_jump_result::jump_type;
					case jump_break:
					{
						patch_jump(node, label, end_label);
						break;
					}
					case jump_continue:
					{
						patch_jump(node, label, continue_label);
						break;
					}// NOLINT(clang-diagnostic-covered-switch-default)
					default:
					{
						UNREACHABLE();
						gal_assert(false, "Unknown loop jump type!");
					}
				}
			}
		}

		register_type new_registers(const ast::ast_node* node, register_size_type needed)
		{
			const auto previous_top = register_top_;

			if (register_top_ + needed > max_register_size) { throw compile_error{node->get_location(), std_format::format("Out of registers when trying to allocate {} registers({} already exist): exceeded limit {}", needed, register_top_, max_register_size)}; }

			register_top_ += needed;
			stack_size_ = std::ranges::max(stack_size_, register_top_);

			return static_cast<register_type>(previous_top);
		}

		void reserve_registers(const ast::ast_node* node, register_size_type needed)
		{
			if (register_top_ + needed > max_register_size)
			{
				throw compile_error{node->get_location(),
				                    std_format::format("Out of registers when trying to allocate {} registers({} already exist): exceeded limit {}", needed, register_top_, max_register_size)};
			}

			stack_size_ = std::ranges::max(stack_size_, register_top_ + needed);
		}

		template<bool End = false>
		constexpr void set_debug_line(const ast::ast_node* node) const noexcept { set_debug_line<End>(node->get_location()); }

		template<bool End = false>
		constexpr void set_debug_line(const utils::location loc) const noexcept
		{
			if (options_.debug_level >= 1)
			{
				if constexpr (End) { bytecode_.set_debug_line(static_cast<int>(loc.end.line + 1)); }
				else { bytecode_.set_debug_line(static_cast<int>(loc.begin.line + 1)); }
			}
		}

		[[nodiscard]] builtin_method get_builtin(ast::ast_expression* node)
		{
			if (auto* local = node->as<ast::ast_expression_local>(); local)
			{
				const auto it = locals_.find(local->get_local());

				return it != locals_.end() && not it->second.written && it->second.init ? get_builtin(it->second.init) : builtin_method{};
			}

			if (auto* index_name = node->as<ast::ast_expression_index_name>(); index_name)
			{
				if (auto* global = index_name->get_expression()->as<ast::ast_expression_global>(); global)
				{
					const auto it = globals_.find(global->get_name());

					return (it == globals_.end() || (not it->second.writable && not it->second.written)) ? builtin_method{global->get_name(), index_name->get_index()} : builtin_method{};
				}
				return {};
			}

			if (auto* global = node->as<ast::ast_expression_global>(); global)
			{
				const auto it = globals_.find(global->get_name());

				return (it == globals_.end() || not it->second.written) ? builtin_method{{}, global->get_name()} : builtin_method{};
			}

			return {};
		}

		[[nodiscard]] constexpr static builtin_method::method_id_type get_builtin_method_id(const builtin_method& method)
		{
			if (method.empty()) { return builtin_method::invalid_method_id; }

			auto make_id = [](builtin_function f) constexpr noexcept { return static_cast<builtin_method::method_id_type>(f); };

			using enum builtin_function;

			if (method.is_global(builtin_assert)) { return make_id(assert); }

			if (method.is_global(builtin_type)) { return make_id(type); }

			if (method.is_global(builtin_typeof)) { return make_id(typeof); }

			if (method.is_global(builtin_raw_set)) { return make_id(raw_set); }
			if (method.is_global(builtin_raw_get)) { return make_id(raw_get); }
			if (method.is_global(builtin_raw_equal)) { return make_id(raw_equal); }

			if (method.is_global(builtin_unpack)) { return make_id(table_unpack); }

			if (method.is_method(builtin_math_prefix, builtin_math_abs)) { return make_id(math_abs); }
			if (method.is_method(builtin_math_prefix, builtin_math_acos)) { return make_id(math_acos); }
			if (method.is_method(builtin_math_prefix, builtin_math_asin)) { return make_id(math_asin); }
			if (method.is_method(builtin_math_prefix, builtin_math_atan2)) { return make_id(math_atan2); }
			if (method.is_method(builtin_math_prefix, builtin_math_atan)) { return make_id(math_atan); }
			if (method.is_method(builtin_math_prefix, builtin_math_ceil)) { return make_id(math_ceil); }
			if (method.is_method(builtin_math_prefix, builtin_math_cosh)) { return make_id(math_cosh); }
			if (method.is_method(builtin_math_prefix, builtin_math_cos)) { return make_id(math_cos); }
			if (method.is_method(builtin_math_prefix, builtin_math_clamp)) { return make_id(math_clamp); }
			if (method.is_method(builtin_math_prefix, builtin_math_deg)) { return make_id(math_deg); }
			if (method.is_method(builtin_math_prefix, builtin_math_exp)) { return make_id(math_exp); }
			if (method.is_method(builtin_math_prefix, builtin_math_floor)) { return make_id(math_floor); }
			if (method.is_method(builtin_math_prefix, builtin_math_fmod)) { return make_id(math_fmod); }
			if (method.is_method(builtin_math_prefix, builtin_math_fexp)) { return make_id(math_fexp); }
			if (method.is_method(builtin_math_prefix, builtin_math_ldexp)) { return make_id(math_ldexp); }
			if (method.is_method(builtin_math_prefix, builtin_math_log10)) { return make_id(math_log10); }
			if (method.is_method(builtin_math_prefix, builtin_math_log)) { return make_id(math_log); }
			if (method.is_method(builtin_math_prefix, builtin_math_max)) { return make_id(math_max); }
			if (method.is_method(builtin_math_prefix, builtin_math_min)) { return make_id(math_min); }
			if (method.is_method(builtin_math_prefix, builtin_math_modf)) { return make_id(math_modf); }
			if (method.is_method(builtin_math_prefix, builtin_math_pow)) { return make_id(math_pow); }
			if (method.is_method(builtin_math_prefix, builtin_math_rad)) { return make_id(math_rad); }
			if (method.is_method(builtin_math_prefix, builtin_math_sign)) { return make_id(math_sign); }
			if (method.is_method(builtin_math_prefix, builtin_math_sinh)) { return make_id(math_sinh); }
			if (method.is_method(builtin_math_prefix, builtin_math_sin)) { return make_id(math_sin); }
			if (method.is_method(builtin_math_prefix, builtin_math_sqrt)) { return make_id(math_sqrt); }
			if (method.is_method(builtin_math_prefix, builtin_math_tanh)) { return make_id(math_tanh); }
			if (method.is_method(builtin_math_prefix, builtin_math_tan)) { return make_id(math_tan); }
			if (method.is_method(builtin_math_prefix, builtin_math_round)) { return make_id(math_round); }

			if (method.is_method(builtin_bits_prefix, builtin_bits_arshift)) { return make_id(bits_arshift); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_and)) { return make_id(bits_and); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_or)) { return make_id(bits_or); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_xor)) { return make_id(bits_xor); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_not)) { return make_id(bits_not); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_test)) { return make_id(bits_test); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_extract)) { return make_id(bits_extract); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_lrotate)) { return make_id(bits_lrotate); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_lshift)) { return make_id(bits_lshift); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_replace)) { return make_id(bits_replace); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_rrotate)) { return make_id(bits_rrotate); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_rshift)) { return make_id(bits_rshift); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_countlz)) { return make_id(bits_countlz); }
			if (method.is_method(builtin_bits_prefix, builtin_bits_countrz)) { return make_id(bits_countrz); }

			if (method.is_method(builtin_string_prefix, builtin_string_byte)) { return make_id(string_byte); }
			if (method.is_method(builtin_string_prefix, builtin_string_char)) { return make_id(string_char); }
			if (method.is_method(builtin_string_prefix, builtin_string_len)) { return make_id(string_len); }
			if (method.is_method(builtin_string_prefix, builtin_string_sub)) { return make_id(string_sub); }

			if (method.is_method(builtin_table_prefix, builtin_table_insert)) { return make_id(table_insert); }
			if (method.is_method(builtin_table_prefix, builtin_table_unpack)) { return make_id(table_unpack); }

			if (method.is_method(builtin_vector_prefix, builtin_vector_ctor)) { return make_id(vector); }

			return builtin_method::invalid_method_id;
		}
	};
}
