#pragma once

#ifndef GAL_LANG_AST_AST_HPP
#define GAL_LANG_AST_AST_HPP

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <optional>
#include <variant>
#include <memory>

#include <utils/point.hpp>
#include <utils/concept.hpp>
#include <utils/macro.hpp>

namespace gal
{
	using gal_boolean_type = bool;
	using gal_number_type = double;
	using gal_string_type = std::string;

	using ast_name = std::string;
	using ast_name_view = std::string_view;

	class ast_visitor;

	class ast_node;
	class ast_expression;
	class ast_statement;
	class ast_statement_block;
	class ast_type;
	class ast_type_pack;

	using ast_expression_type = std::unique_ptr<ast_expression>;

	struct ast_local
	{
		ast_name name;
		location loc;

		ast_local* shadow;
		std::size_t function_depth;
		std::size_t loop_depth;

		ast_type* annotation;

		void visit(ast_visitor& visitor) const;

		[[nodiscard]] const ast_name& get_name() const noexcept { return name; }
	};

	template<typename T>
	using ast_array = std::vector<T>;

	struct ast_type_list
	{
		ast_array<ast_type*> types;
		// nullptr indicates no tail, not an untyped tail.
		ast_type_pack* tail_type = nullptr;

		void visit(ast_visitor& visitor) const;
	};

	struct ast_argument_name
	{
		ast_name name;
		location loc;
	};

	class ast_visitor
	{
	public:
		constexpr ast_visitor() = default;
		constexpr virtual ~ast_visitor() = 0;
		constexpr ast_visitor(const ast_visitor&) = default;
		constexpr ast_visitor& operator=(const ast_visitor&) = default;
		constexpr ast_visitor(ast_visitor&&) = default;
		constexpr ast_visitor& operator=(ast_visitor&&) = default;

		virtual bool visit(ast_node&) { return true; }
	};

	using ast_rtti_index_type = int;

	struct ast_rtti_index_counter
	{
		inline static ast_rtti_index_type index = 0;
	};

	template<typename T>
	struct ast_rtti
	{
		const static ast_rtti_index_type value;
	};

	template<typename T>
	const ast_rtti_index_type ast_rtti<T>::value = ++ast_rtti_index_counter::index;

	#define GAL_SET_RTTI(class_name) \
	constexpr static auto get_rtti_index() noexcept \
	{ \
		return ast_rtti<class_name>::value; \
	}

	template<typename T>
	concept has_rtti_index = requires(T t)
	{
		{
			T::get_rtti_index()
		} -> std::same_as<ast_rtti_index_type>;
	};

	class ast_node
	{
	protected:
		ast_rtti_index_type class_index_;
		location loc_;

	public:
		constexpr ast_node(const ast_rtti_index_type class_index, location loc)
			: class_index_{class_index},
			  loc_{loc} {}

		constexpr virtual ~ast_node() = default;
		constexpr ast_node(const ast_node&) = default;
		constexpr ast_node& operator=(const ast_node&) = default;
		constexpr ast_node(ast_node&&) = default;
		constexpr ast_node& operator=(ast_node&&) = default;

		virtual void visit(ast_visitor& visitor) = 0;

		virtual ast_expression* as_expression() { return nullptr; }

		virtual ast_statement* as_statement() { return nullptr; }

		virtual ast_type* as_type() { return nullptr; }

		template<has_rtti_index T>
		[[nodiscard]] constexpr bool is() const noexcept { return class_index_ == T::get_rtti_index(); }

		template<has_rtti_index T>
		[[nodiscard]] constexpr T* as() noexcept { return class_index_ == T::get_rtti_index() ? static_cast<T*>(this) : nullptr; }

		template<has_rtti_index T>
		[[nodiscard]] constexpr const T* as() const noexcept { return class_index_ == T::get_rtti_index() ? static_cast<const T*>(this) : nullptr; }
	};

	class ast_expression : public ast_node
	{
	public:
		using ast_node::ast_node;

		ast_expression* as_expression() override { return this; }

		[[nodiscard]] ast_name		get_identifier() const noexcept;
	};

	class ast_expression_error final : public ast_expression
	{
	public:
		using error_expressions_type = ast_array<ast_expression*>;

	private:
		error_expressions_type expressions_;
		unsigned message_index_;

	public:
		GAL_SET_RTTI(ast_expression_error)

		ast_expression_error(const location loc, error_expressions_type expressions, unsigned message_index)
			: ast_expression{get_rtti_index(), loc},
			  expressions_{std::move(expressions)},
			  message_index_{message_index} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { for (const auto& expression: expressions_) { expression->visit(visitor); } } }

		// todo: interface
	};

	class ast_statement : public ast_node
	{
	private:
		bool has_semicolon_{false};

	public:
		using ast_node::ast_node;

		ast_statement* as_statement() override { return this; }

		[[nodiscard]] constexpr bool has_semicolon() const noexcept { return has_semicolon_; }

		constexpr void set_semicolon(const bool has) noexcept { has_semicolon_ = has; }
	};

	class ast_statement_block final : public ast_statement
	{
	public:
		using block_body_type = ast_array<ast_statement*>;

	private:
		block_body_type body_;

	public:
		GAL_SET_RTTI(ast_statement_block)

		ast_statement_block(const location loc, block_body_type body)
			: ast_statement{get_rtti_index(), loc},
			  body_{std::move(body)} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { for (const auto& statement: body_) { statement->visit(visitor); } } }
	};

	class ast_statement_error final : public ast_statement
	{
	public:
		using error_statements_type = ast_array<ast_statement*>;
		using error_expressions_type = ast_expression_error::error_expressions_type;

	private:
		error_expressions_type expressions_;
		error_statements_type statements_;
		unsigned message_index_;

	public:
		GAL_SET_RTTI(ast_statement_error)

		ast_statement_error(const location loc, error_expressions_type expressions, error_statements_type statements, const unsigned message_index)
			: ast_statement{get_rtti_index(), loc},
			  expressions_{std::move(expressions)},
			  statements_{std::move(statements)},
			  message_index_{message_index} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				for (const auto& expression: expressions_) { expression->visit(visitor); }

				for (const auto& statement: statements_) { statement->visit(visitor); }
			}
		}

		// todo: interface
	};

	class ast_type : public ast_node
	{
	public:
		using ast_node::ast_node;

		ast_type* as_type() override { return this; }
	};

	class ast_type_pack : public ast_node
	{
	public:
		using ast_node::ast_node;
	};

	class ast_type_error final : public ast_type
	{
	public:
		using error_types_type = ast_array<ast_type*>;

	private:
		error_types_type types_;
		bool is_missing_;
		unsigned message_index_;

	public:
		GAL_SET_RTTI(ast_type_error)

		ast_type_error(const location loc, error_types_type types, bool is_missing, unsigned message_index)
			: ast_type{get_rtti_index(), loc},
			  types_{std::move(types)},
			  is_missing_{is_missing},
			  message_index_{message_index} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { for (const auto& type: types_) { type->visit(visitor); } } }

		// todo: interface
	};

	class ast_type_pack_explicit final : public ast_type_pack
	{
	private:
		ast_type_list types_;

	public:
		GAL_SET_RTTI(ast_type_pack_explicit)

		ast_type_pack_explicit(const location loc, ast_type_list types)
			: ast_type_pack{get_rtti_index(), loc},
			  types_{std::move(types)} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { types_.visit(visitor); } }

		// todo: interface
	};

	class ast_type_pack_variadic final : public ast_type_pack
	{
	private:
		ast_type* variadic_type_;

	public:
		GAL_SET_RTTI(ast_type_pack_variadic)

		constexpr ast_type_pack_variadic(const location loc, ast_type* variadic_type)
			: ast_type_pack{get_rtti_index(), loc},
			  variadic_type_{variadic_type} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { variadic_type_->visit(visitor); } }

		// todo: interface
	};

	class ast_type_pack_generic final : public ast_type_pack
	{
	private:
		ast_name generic_name_;

	public:
		GAL_SET_RTTI(ast_type_pack_generic)

		ast_type_pack_generic(const location loc, ast_name generic_name)
			: ast_type_pack{get_rtti_index(), loc},
			  generic_name_{std::move(generic_name)} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }

		// todo: interface
	};

	class ast_type_or_pack final
	{
		std::variant<ast_type*, ast_type_pack*> value_;

	public:
		template<typename T>
			requires is_any_type_of_v<T, ast_type, ast_type_pack>
		[[nodiscard]] constexpr bool holding() const noexcept { return std::holds_alternative<T*>(value_); }

		template<typename Callable>
		constexpr auto visit(Callable&& callable) { return std::visit(std::forward<Callable>(callable), value_); }

		template<typename Callable>
		constexpr auto visit(Callable&& callable) const { return std::visit(std::forward<Callable>(callable), value_); }
	};

	class ast_type_reference final : public ast_type
	{
	public:
		using parameter_types_type = ast_array<ast_type_or_pack>;

	private:
		ast_name name_;
		std::optional<ast_name> prefix_;
		std::optional<parameter_types_type> parameters_;

	public:
		GAL_SET_RTTI(ast_type_reference)

		ast_type_reference(
				const location loc,
				ast_name name,
				std::optional<ast_name> prefix,
				std::optional<parameter_types_type> parameters = std::nullopt
				)
			: ast_type{get_rtti_index(), loc},
			  name_{std::move(name)},
			  prefix_{prefix.has_value() ? std::move(prefix) : ast_name{}},
			  parameters_{std::move(parameters)} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { for (const auto& parameter: parameters_.value()) { parameter.visit([&visitor](const auto& type) { type->visit(visitor); }); } } }

		// todo: interface
	};

	class ast_type_table final : public ast_type
	{
	public:
		struct ast_table_property
		{
			ast_name name;
			location loc;
			ast_type* type;

			void visit(ast_visitor& visitor) const { type->visit(visitor); }
		};

		struct ast_table_indexer
		{
			ast_type* index_type;
			ast_type* result_type;
			location loc;

			void visit(ast_visitor& visitor) const
			{
				index_type->visit(visitor);
				result_type->visit(visitor);
			}
		};

		using table_properties_type = ast_array<ast_table_property>;

	private:
		table_properties_type properties_;
		ast_table_indexer* indexer_;

	public:
		GAL_SET_RTTI(ast_type_table)

		ast_type_table(const location loc, table_properties_type properties, ast_table_indexer* indexer = nullptr)
			: ast_type{get_rtti_index(), loc},
			  properties_{std::move(properties)},
			  indexer_{indexer} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				for (const auto& property: properties_) { property.visit(visitor); }

				if (indexer_) { indexer_->visit(visitor); }
			}
		}

		// todo: interface
	};

	class ast_type_function final : public ast_type
	{
	public:
		using generic_names_type = ast_array<ast_name>;
		using argument_names_type = ast_array<std::optional<ast_argument_name>>;

	private:
		generic_names_type generics_;
		generic_names_type generic_packs_;
		ast_type_list arg_types_;
		argument_names_type arg_names_;
		ast_type_list return_types_;

	public:
		GAL_SET_RTTI(ast_type_function)

		ast_type_function(
				const location loc,
				generic_names_type generics,
				generic_names_type generic_packs,
				ast_type_list arg_types,
				argument_names_type arg_names,
				ast_type_list return_types
				)
			: ast_type{get_rtti_index(), loc},
			  generics_{std::move(generics)},
			  generic_packs_{std::move(generic_packs)},
			  arg_types_{std::move(arg_types)},
			  arg_names_{std::move(arg_names)},
			  return_types_{std::move(return_types)} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				arg_types_.visit(visitor);
				return_types_.visit(visitor);
			}
		}

		// todo: interface
	};

	class ast_type_typeof final : ast_type
	{
	private:
		ast_expression* expression_;

	public:
		GAL_SET_RTTI(ast_type_typeof)

		constexpr ast_type_typeof(const location loc, ast_expression* expression)
			: ast_type{get_rtti_index(), loc},
			  expression_{expression} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { expression_->visit(visitor); } }

		// todo: interface
	};

	class ast_type_union final : public ast_type
	{
	public:
		using union_types_type = ast_array<ast_type*>;

	private:
		union_types_type types_;

	public:
		GAL_SET_RTTI(ast_type_union)

		ast_type_union(const location loc, union_types_type types)
			: ast_type{get_rtti_index(), loc},
			  types_{std::move(types)} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { for (const auto& type: types_) { type->visit(visitor); } } }

		// todo: interface
	};

	class ast_type_intersection final : public ast_type
	{
	public:
		using intersection_types_type = ast_array<ast_type*>;

	private:
		intersection_types_type types_;

	public:
		GAL_SET_RTTI(ast_type_intersection)

		ast_type_intersection(const location loc, intersection_types_type types)
			: ast_type{get_rtti_index(), loc},
			  types_{std::move(types)} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { for (const auto& type: types_) { type->visit(visitor); } } }

		// todo: interface
	};

	class ast_type_singleton_boolean final : public ast_type
	{
	private:
		gal_boolean_type value_;

	public:
		GAL_SET_RTTI(ast_type_singleton_boolean)

		constexpr ast_type_singleton_boolean(const location loc, gal_boolean_type value)
			: ast_type{get_rtti_index(), loc},
			  value_{value} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }

		// todo: interface
	};

	class ast_type_singleton_string final : public ast_type
	{
	private:
		gal_string_type value_;

	public:
		GAL_SET_RTTI(ast_type_singleton_string)

		ast_type_singleton_string(const location loc, gal_string_type value)
			: ast_type{get_rtti_index(), loc},
			  value_{std::move(value)} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }

		// todo: interface
	};

	class ast_expression_group final : public ast_expression
	{
	private:
		ast_expression* expression_;

	public:
		GAL_SET_RTTI(ast_expression_group)

		constexpr ast_expression_group(const location loc, ast_expression* expression)
			: ast_expression{get_rtti_index(), loc},
			  expression_{expression} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { expression_->visit(visitor); } }
	};

	class ast_expression_constant_null final : public ast_expression
	{
	public:
		GAL_SET_RTTI(ast_expression_constant_null)

		constexpr explicit ast_expression_constant_null(const location loc)
			: ast_expression{get_rtti_index(), loc} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }
	};

	class ast_expression_constant_boolean final : public ast_expression
	{
	private:
		gal_boolean_type value_;

	public:
		GAL_SET_RTTI(ast_expression_constant_boolean)

		constexpr ast_expression_constant_boolean(const location loc, const gal_boolean_type value)
			: ast_expression{get_rtti_index(), loc},
			  value_{value} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }

		[[nodiscard]] auto get() const noexcept { return value_; }
	};

	class ast_expression_constant_number final : public ast_expression
	{
	private:
		gal_number_type value_;

	public:
		GAL_SET_RTTI(ast_expression_constant_number)

		constexpr ast_expression_constant_number(const location loc, const gal_number_type value)
			: ast_expression{get_rtti_index(), loc},
			  value_{value} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }

		[[nodiscard]] auto get() const noexcept { return value_; }
	};

	class ast_expression_constant_string final : public ast_expression
	{
	private:
		gal_string_type value_;

	public:
		GAL_SET_RTTI(ast_expression_constant_string)

		ast_expression_constant_string(const location loc, gal_string_type value)
			: ast_expression{get_rtti_index(), loc},
			  value_{std::move(value)} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }
	};

	class ast_expression_local final : public ast_expression
	{
	private:
		ast_local* local_;
		bool is_upvalue_;

	public:
		GAL_SET_RTTI(ast_expression_local)

		constexpr ast_expression_local(const location loc, ast_local* local, const bool is_upvalue)
			: ast_expression{get_rtti_index(), loc},
			  local_{local},
			  is_upvalue_{is_upvalue} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }

		[[nodiscard]] constexpr auto get_local() noexcept { return local_; }

		[[nodiscard]] constexpr auto get_local() const noexcept { return local_; }

		[[nodiscard]] constexpr bool is_upvalue() const noexcept { return is_upvalue_; }
	};

	class ast_expression_global final : public ast_expression
	{
	private:
		ast_name name_;

	public:
		GAL_SET_RTTI(ast_expression_global)

		ast_expression_global(const location loc, ast_name name)
			: ast_expression{get_rtti_index(), loc},
			  name_{std::move(name)} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }

		[[nodiscard]] const ast_name& get_name() const noexcept { return name_; }
	};

	class ast_expression_varargs final : public ast_expression
	{
	public:
		GAL_SET_RTTI(ast_expression_varargs)

		constexpr explicit ast_expression_varargs(const location loc)
			: ast_expression{get_rtti_index(), loc} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }
	};

	class ast_expression_call final : public ast_expression
	{
	public:
		using call_args_type = ast_array<ast_expression*>;

	private:
		ast_expression* function_;
		call_args_type args_;
		bool is_self_;
		location arg_loc_;

	public:
		GAL_SET_RTTI(ast_expression_call)

		ast_expression_call(const location loc, ast_expression* function, call_args_type args, const bool is_self, const location arg_loc)
			: ast_expression{get_rtti_index(), loc},
			  function_{function},
			  args_{std::move(args)},
			  is_self_{is_self},
			  arg_loc_{arg_loc} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				function_->visit(visitor);

				for (const auto& arg: args_) { arg->visit(visitor); }
			}
		}

		// todo: interface
	};

	class ast_expression_index_name final : public ast_expression
	{
	private:
		ast_expression* expression_;
		ast_name index_;
		location index_loc_;
		position operand_pos_;
		char operand_;

	public:
		GAL_SET_RTTI(ast_expression_index_name)

		ast_expression_index_name(const location loc, ast_expression* expression, ast_name index, const location index_loc, const position operand_pos, const char operand)
			: ast_expression{get_rtti_index(), loc},
			  expression_{expression},
			  index_{std::move(index)},
			  index_loc_{index_loc},
			  operand_pos_{operand_pos},
			  operand_{operand} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { expression_->visit(visitor); } }

		// todo: interface
	};

	class ast_expression_function final : public ast_expression
	{
	public:
		using generics_args_type = ast_array<ast_name>;
		using args_locals_type = ast_array<ast_local*>;

	private:
		generics_args_type generics_;
		generics_args_type generic_packs_;
		ast_local* self_;
		args_locals_type args_;

		std::optional<location> vararg_loc_;
		ast_type_pack* vararg_annotation_;

		ast_statement_block* body_;

		std::size_t function_depth_;

		ast_name debug_name_;

		std::optional<ast_type_list> return_annotation_;

		bool has_end_;

		std::optional<location> arg_location_;

	public:
		GAL_SET_RTTI(ast_expression_function)

		ast_expression_function(
				const location loc,
				generics_args_type generics,
				generics_args_type generic_packs,
				ast_local* self,
				args_locals_type args,
				std::optional<location> vararg_loc,
				ast_statement_block* body,
				const std::size_t function_depth,
				ast_name debug_name,
				std::optional<ast_type_list> return_annotation = std::nullopt,
				ast_type_pack* vararg_annotation = nullptr,
				const bool has_end = false,
				std::optional<location> arg_location = std::nullopt
				)
			: ast_expression{get_rtti_index(), loc},
			  generics_{std::move(generics)},
			  generic_packs_{std::move(generic_packs)},
			  self_{self},
			  args_{std::move(args)},
			  vararg_loc_{vararg_loc.value_or(location{})},
			  vararg_annotation_{vararg_annotation},
			  body_{body},
			  function_depth_{function_depth},
			  debug_name_{std::move(debug_name)},
			  return_annotation_{std::move(return_annotation)},
			  has_end_{has_end},
			  arg_location_{arg_location} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				for (const auto& arg: args_) { arg->visit(visitor); }

				if (vararg_annotation_) { vararg_annotation_->visit(visitor); }

				if (return_annotation_) { return_annotation_->visit(visitor); }

				body_->visit(visitor);
			}
		}

		// todo: interface
	};

	class ast_expression_table final : public ast_expression
	{
	public:
		struct item
		{
			enum class item_type
			{
				// value only
				list,
				// key is a ast_expression_constant_string
				record,
				general,
			};

			// todo: use set & map

			item_type type;
			std::pair<ast_expression*, ast_expression*> kv;
		};

		using items_type = ast_array<item>;

	private:
		items_type items_;

	public:
		GAL_SET_RTTI(ast_expression_table)

		ast_expression_table(const location loc, items_type items)
			: ast_expression{get_rtti_index(), loc},
			  items_{std::move(items)} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				for (const auto& [_, kv]: items_)
				{
					const auto& [key, value] = kv;
					if (key) { key->visit(visitor); }
					value->visit(visitor);
				}
			}
		}

		// todo: interface
	};

	class ast_expression_unary final : public ast_expression
	{
	public:
		enum class operand_type
		{
			// +
			unary_plus,
			// -
			unary_minus,
			// !
			unary_not,
			// todo: more unary operand ?
		};

	private:
		operand_type operand_;
		ast_expression* expression_;

	public:
		GAL_SET_RTTI(ast_expression_unary)

		constexpr ast_expression_unary(const location loc, const operand_type operand, ast_expression* expression)
			: ast_expression{get_rtti_index(), loc},
			  operand_{operand},
			  expression_{expression} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { expression_->visit(visitor); } }

		[[nodiscard]] std::string operand() const noexcept
		{
			switch (operand_)
			{
				case operand_type::unary_plus: { return "+"; }
				case operand_type::unary_minus: { return "-"; }
				case operand_type::unary_not: { return "not"; }
			}

			UNREACHABLE();
		}

		// todo: interface
	};

	class ast_expression_binary final : public ast_expression
	{
	public:
		enum class operand_type
		{
			// +
			plus,
			// -
			minus,
			// *
			multiply,
			// /
			divide,
			// %
			modulus,
			// **
			pow,
			// &
			bitwise_and,
			// |
			bitwise_or,
			// ^
			bitwise_xor,
			// and
			logical_and,
			// or
			logical_or,
			// ==
			equal,
			// !=
			not_equal,
			// <
			less_than,
			// <=
			less_equal,
			// >
			greater_than,
			// >=
			greater_equal,
			// todo: more unary operand ?
		};

	private:
		operand_type operand_;
		ast_expression* lhs_;
		ast_expression* rhs_;

	public:
		GAL_SET_RTTI(ast_expression_binary)

		constexpr ast_expression_binary(const location loc, const operand_type operand, ast_expression* lhs, ast_expression* rhs)
			: ast_expression{get_rtti_index(), loc},
			  operand_{operand},
			  lhs_{lhs},
			  rhs_{rhs} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				lhs_->visit(visitor);
				rhs_->visit(visitor);
			}
		}

		[[nodiscard]] std::string operand() const noexcept
		{
			switch (operand_)
			{
				case operand_type::plus: { return "+"; }
				case operand_type::minus: { return "-"; }
				case operand_type::multiply: { return "*"; }
				case operand_type::divide: { return "/"; }
				case operand_type::modulus: { return "%"; }
				case operand_type::pow: { return "**"; }
				case operand_type::bitwise_and: { return "&"; }
				case operand_type::bitwise_or: { return "|"; }
				case operand_type::bitwise_xor: { return "^"; }
				case operand_type::logical_and: { return "and"; }
				case operand_type::logical_or: { return "or"; }
				case operand_type::equal: { return "=="; }
				case operand_type::not_equal: { return "!="; }
				case operand_type::less_than: { return "<"; }
				case operand_type::less_equal: { return "<="; }
				case operand_type::greater_than: { return ">"; }
				case operand_type::greater_equal: { return ">="; }
			}

			UNREACHABLE();
		}

		// todo: interface
	};

	class ast_expression_type_assertion final : public ast_expression
	{
	private:
		ast_expression* expression_;
		ast_type* annotation_;

	public:
		GAL_SET_RTTI(ast_expression_type_assertion)

		constexpr ast_expression_type_assertion(const location loc, ast_expression* expression, ast_type* annotation)
			: ast_expression{get_rtti_index(), loc},
			  expression_{expression},
			  annotation_{annotation} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				expression_->visit(visitor);
				annotation_->visit(visitor);
			}
		}

		// todo: interface
	};

	class ast_expression_if_else final : public ast_expression
	{
	private:
		bool has_then_;
		bool has_else_;

		ast_expression* condition_;
		ast_expression* true_expression_;
		ast_expression* false_expression_;

	public:
		GAL_SET_RTTI(ast_expression_if_else)

		constexpr ast_expression_if_else(
				const location loc,
				const bool has_then,
				const bool has_else,
				ast_expression* condition,
				ast_expression* true_expression,
				ast_expression* false_expression)
			: ast_expression{get_rtti_index(), loc},
			  has_then_{has_then},
			  has_else_{has_else},
			  condition_{condition},
			  true_expression_{true_expression},
			  false_expression_{false_expression} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				condition_->visit(visitor);
				true_expression_->visit(visitor);
				false_expression_->visit(visitor);
			}
		}

		// todo: interface
	};

	class ast_statement_if final : public ast_statement
	{
	private:
		ast_expression* condition_;
		ast_statement_block* then_body_;
		ast_statement* else_body_;

		std::optional<location> then_loc_;

		std::optional<location> else_loc_;

		bool has_end_;

	public:
		GAL_SET_RTTI(ast_statement_if)

		constexpr ast_statement_if(
				const location loc,
				ast_expression* condition,
				ast_statement_block* then_body,
				ast_statement* else_body,
				const std::optional<location> then_loc,
				const std::optional<location> else_loc,
				const bool has_end
				)
			: ast_statement{get_rtti_index(), loc},
			  condition_{condition},
			  then_body_{then_body},
			  else_body_{else_body},
			  then_loc_{then_loc},
			  else_loc_{else_loc},
			  has_end_{has_end} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				condition_->visit(visitor);
				then_body_->visit(visitor);
				if (else_body_) { else_body_->visit(visitor); }
			}
		}

		// todo: interface
	};

	class ast_statement_while final : public ast_statement
	{
	private:
		ast_expression* condition_;
		ast_statement_block* body_;

		std::optional<location> do_loc_;

		bool has_end_;

	public:
		GAL_SET_RTTI(ast_statement_while)

		constexpr ast_statement_while(const location loc,
		                              ast_expression* condition,
		                              ast_statement_block* body,
		                              std::optional<location> do_loc,
		                              const bool has_end)
			: ast_statement{get_rtti_index(), loc},
			  condition_{condition},
			  body_{body},
			  do_loc_{do_loc},
			  has_end_{has_end} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				condition_->visit(visitor);
				body_->visit(visitor);
			}
		}

		// todo: interface
	};

	class ast_statement_repeat final : public ast_statement
	{
	private:
		ast_expression* condition_;
		ast_statement_block* body_;

		bool has_until_;

	public:
		GAL_SET_RTTI(ast_statement_repeat)

		constexpr ast_statement_repeat(const location loc, ast_expression* condition, ast_statement_block* body, bool has_until)
			: ast_statement{get_rtti_index(), loc},
			  condition_{condition},
			  body_{body},
			  has_until_{has_until} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				condition_->visit(visitor);
				body_->visit(visitor);
			}
		}

		// todo: interface
	};

	class ast_statement_for final : public ast_statement
	{
	private:
		ast_local* var_;

		ast_expression* begin_;
		ast_expression* end_;
		ast_expression* step_;

		ast_statement_block* body_;

		std::optional<location> do_loc_;

		bool has_end_;

	public:
		GAL_SET_RTTI(ast_statement_for)

		constexpr ast_statement_for(
				const location loc,
				ast_expression* begin,
				ast_expression* end,
				ast_expression* step,
				ast_statement_block* body,
				std::optional<location> do_loc,
				const bool has_end)
			: ast_statement{get_rtti_index(), loc},
			  begin_{begin},
			  end_{end},
			  step_{step},
			  body_{body},
			  do_loc_{do_loc},
			  has_end_{has_end} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this)) { var_->visit(visitor); }

			begin_->visit(visitor);
			end_->visit(visitor);

			if (step_) { step_->visit(visitor); }

			body_->visit(visitor);
		}

		// todo: interface
	};

	class ast_statement_for_in final : public ast_statement
	{
	public:
		using var_locals_type = ast_array<ast_local*>;
		using value_expressions_type = ast_array<ast_expression*>;

	private:
		var_locals_type vars_;
		value_expressions_type values_;
		ast_statement_block* body_;

		std::optional<location> in_loc_;

		std::optional<location> do_loc_;

		bool has_end_;

	public:
		GAL_SET_RTTI(ast_statement_for_in)

		ast_statement_for_in(const location loc,
		                     var_locals_type vars,
		                     value_expressions_type values,
		                     ast_statement_block* body,
		                     std::optional<location> in_loc,
		                     std::optional<location> do_loc,
		                     const bool has_end)
			: ast_statement{get_rtti_index(), loc},
			  vars_{std::move(vars)},
			  values_{std::move(values)},
			  body_{body},
			  in_loc_{in_loc},
			  do_loc_{do_loc},
			  has_end_{has_end} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				for (const auto& var: vars_) { var->visit(visitor); }

				for (const auto& value: values_) { value->visit(visitor); }

				body_->visit(visitor);
			}
		}

		// todo: interface
	};

	class ast_statement_break final : public ast_statement
	{
	public:
		GAL_SET_RTTI(ast_statement_break)

		constexpr explicit ast_statement_break(const location loc)
			: ast_statement{get_rtti_index(), loc} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }
	};

	class ast_statement_continue final : public ast_statement
	{
	public:
		GAL_SET_RTTI(ast_statement_continue)

		constexpr explicit ast_statement_continue(const location loc)
			: ast_statement{get_rtti_index(), loc} {}

		void visit(ast_visitor& visitor) override { visitor.visit(*this); }

		// todo: interface
	};

	class ast_statement_return final : public ast_statement
	{
	public:
		using expression_list_type = ast_array<ast_expression*>;

	private:
		expression_list_type list_;

	public:
		GAL_SET_RTTI(ast_statement_return)

		ast_statement_return(const location loc, expression_list_type list)
			: ast_statement{get_rtti_index(), loc},
			  list_{std::move(list)} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { for (const auto& expression: list_) { expression->visit(visitor); } } }

		// todo: interface
	};

	class ast_statement_expression final : public ast_statement
	{
	private:
		ast_expression* expression_;

	public:
		GAL_SET_RTTI(ast_statement_expression)

		constexpr ast_statement_expression(const location loc, ast_expression* expression)
			: ast_statement{get_rtti_index(), loc},
			  expression_{expression} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { expression_->visit(visitor); } }

		// todo: interface
	};

	class ast_statement_local final : public ast_statement
	{
	public:
		using var_locals_type = ast_array<ast_local*>;
		using value_expressions_type = ast_array<ast_expression*>;

	private:
		var_locals_type vars_;
		value_expressions_type values_;

		std::optional<location> assignment_loc_;

	public:
		GAL_SET_RTTI(ast_statement_local)

		ast_statement_local(const location loc,
		                    var_locals_type vars,
		                    value_expressions_type values,
		                    const std::optional<location> assignment_loc)
			: ast_statement{get_rtti_index(), loc},
			  vars_{std::move(vars)},
			  values_{std::move(values)},
			  assignment_loc_{assignment_loc.value_or(location{})} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				for (const auto& var: vars_) { var->visit(visitor); }

				for (const auto& value: values_) { value->visit(visitor); }
			}
		}

		// todo: interface
	};

	class ast_statement_assign final : public ast_statement
	{
	public:
		using var_expressions_type = ast_array<ast_expression*>;
		using value_expressions_type = ast_array<ast_expression*>;

	private:
		var_expressions_type vars_;
		value_expressions_type values_;

	public:
		GAL_SET_RTTI(ast_statement_assign)

		ast_statement_assign(const location loc, var_expressions_type vars, value_expressions_type values)
			: ast_statement{get_rtti_index(), loc},
			  vars_{std::move(vars)},
			  values_{std::move(values)} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				for (const auto& var: vars_) { var->visit(visitor); }

				for (const auto& value: values_) { value->visit(visitor); }
			}
		}

		// todo: interface
	};

	class ast_statement_compound_assign final : public ast_statement
	{
	public:
		using operand_type = ast_expression_binary::operand_type;

	private:
		operand_type operand_;
		ast_expression* var_;
		ast_expression* value_;

	public:
		GAL_SET_RTTI(ast_statement_compound_assign)

		constexpr ast_statement_compound_assign(const location loc, const operand_type operand, ast_expression* var, ast_expression* value)
			: ast_statement{get_rtti_index(), loc},
			  operand_{operand},
			  var_{var},
			  value_{value} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				var_->visit(visitor);
				value_->visit(visitor);
			}
		}

		// todo: interface
	};

	class ast_statement_function : public ast_statement
	{
	protected:
		ast_expression* name_;
		ast_expression_function* function_;

	public:
		GAL_SET_RTTI(ast_statement_function)

		constexpr ast_statement_function(const location loc, ast_expression* name, ast_expression_function* function)
			: ast_statement{get_rtti_index(), loc},
			  name_{name},
			  function_{function} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				name_->visit(visitor);
				function_->visit(visitor);
			}
		}

		// todo: interface
	};

	class ast_statement_function_local final : public ast_statement_function
	{
	public:
		GAL_SET_RTTI(ast_statement_function_local)

		using ast_statement_function::ast_statement_function;

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { function_->visit(visitor); } }

		// todo: interface
	};

	class ast_statement_type_alias final : public ast_statement
	{
	public:
		using generic_names_type = ast_array<ast_name>;

	private:
		ast_name name_;
		generic_names_type generics_;
		generic_names_type generic_packs_;
		ast_type* type_;
		bool exported_;

	public:
		GAL_SET_RTTI(ast_statement_type_alias)

		ast_statement_type_alias(const location loc,
		                         ast_name name,
		                         generic_names_type generics,
		                         generic_names_type generic_packs,
		                         ast_type* type,
		                         const bool exported)
			: ast_statement{get_rtti_index(), loc},
			  name_{std::move(name)},
			  generics_{std::move(generics)},
			  generic_packs_{std::move(generic_packs)},
			  type_{type},
			  exported_{exported} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { type_->visit(visitor); } }

		// todo: interface
	};

	class ast_statement_declare_global final : public ast_statement
	{
	private:
		ast_name name_;
		ast_type* type_;

	public:
		GAL_SET_RTTI(ast_statement_declare_global)

		ast_statement_declare_global(const location loc, ast_name name, ast_type* type)
			: ast_statement{get_rtti_index(), loc},
			  name_{std::move(name)},
			  type_{type} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { type_->visit(visitor); } }

		// todo: interface
	};

	class ast_statement_declare_function final : public ast_statement
	{
	public:
		using generic_names_type = ast_array<ast_name>;
		using arguments_type = ast_array<ast_argument_name>;

	private:
		ast_name name_;
		generic_names_type generics_;
		generic_names_type generic_packs_;
		ast_type_list params_;
		arguments_type param_names_;
		ast_type_list return_types_;

	public:
		GAL_SET_RTTI(ast_statement_declare_function)

		ast_statement_declare_function(
				const location loc,
				ast_name name,
				generic_names_type generics,
				generic_names_type generic_packs,
				ast_type_list params,
				arguments_type param_names,
				ast_type_list return_type
				)
			: ast_statement{get_rtti_index(), loc},
			  name_{std::move(name)},
			  generics_{std::move(generics)},
			  generic_packs_{std::move(generic_packs)},
			  params_{std::move(params)},
			  param_names_{std::move(param_names)},
			  return_types_{std::move(return_type)} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				params_.visit(visitor);
				return_types_.visit(visitor);
			}
		}

		// todo: interface
	};

	class ast_statement_declare_class final : public ast_statement
	{
	public:
		struct ast_declared_class_property
		{
			ast_name name{};
			ast_type* type{nullptr};
			bool is_method{false};

			void visit(ast_visitor& visitor) const { type->visit(visitor); }
		};

		using class_properties_type = ast_array<ast_declared_class_property>;

	private:
		ast_name name_;
		std::optional<ast_name> super_name_;
		class_properties_type properties_;

	public:
		GAL_SET_RTTI(ast_statement_declare_class)

		ast_statement_declare_class(
				const location loc,
				ast_name name,
				std::optional<ast_name> super_name,
				class_properties_type properties
				)
			: ast_statement{get_rtti_index(), loc},
			  name_{std::move(name)},
			  super_name_{std::move(super_name)},
			  properties_{std::move(properties)} {}

		void visit(ast_visitor& visitor) override { if (visitor.visit(*this)) { for (const auto& property: properties_) { property.visit(visitor); } } }

		// todo: interface
	};
}

#endif // GAL_LANG_AST_AST_HPP
