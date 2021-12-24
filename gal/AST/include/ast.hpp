#pragma once

#ifndef GAL_LANG_AST_AST_HPP
#define GAL_LANG_AST_AST_HPP

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <variant>

#include <utils/point.hpp>
#include <utils/concept.hpp>
#include <utils/macro.hpp>

namespace gal
{
	using gal_boolean_type = bool;
	using gal_number_type = double;
	using gal_string_type = std::string;

	using ast_name = std::string;

	class ast_node;
	class ast_expression;
	class ast_statement;
	class ast_statement_block;
	class ast_type;
	class ast_type_pack;

	struct ast_local
	{
		ast_name name;
		location loc;

		ast_local* shadow;
		std::size_t function_depth;
		std::size_t loop_depth;

		ast_type* annotation;
	};

	template<typename T>
	using ast_array = std::vector<T>;

	struct ast_type_list
	{
		ast_array<ast_type*> types;
		// nullptr indicates no tail, not an untyped tail.
		ast_type_pack* tail_type = nullptr;
	};

	struct ast_arg_name
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

		void visit(const ast_type_list& list);
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
		[[nodiscard]] constexpr T* as() const noexcept { return class_index_ == T::get_rtti_index() ? static_cast<const T*>(this) : nullptr; }
	};

	class ast_expression : public ast_node
	{
	public:
		using ast_node::ast_node;

		ast_expression* as_expression() override { return this; }
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

	struct ast_type_or_pack
	{
		std::variant<ast_type*, ast_type_pack*> value;

		template<typename T>
			requires is_any_type_of_v<T, ast_type, ast_type_pack>
		[[nodiscard]] constexpr bool holding() const noexcept { return std::holds_alternative<T*>(value); }

		template<typename Callable>
		constexpr auto visit(Callable&& callable) { return std::visit(std::forward<Callable>(callable), value); }
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

	class ast_expression_function : public ast_expression
	{
	public:
		using generics_args_type = ast_array<ast_name>;
		using args_locals_type = ast_array<ast_local*>;

	private:
		generics_args_type generics_;
		generics_args_type generic_packs_;
		ast_local* self_;
		args_locals_type args_;

		bool has_vararg_;
		location vararg_loc_;
		ast_type_pack* vararg_annotation_;

		ast_statement_block* body_;

		std::size_t function_depth_;

		ast_name debug_name_;

		bool has_return_annotation_;
		ast_type_list return_annotation_;

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
				std::optional<location> vararg,
				ast_statement_block* body,
				std::size_t function_depth,
				ast_name debug_name,
				std::optional<ast_type_list> return_annotation = std::nullopt,
				ast_type_pack* vararg_annotation = nullptr,
				bool has_end = false,
				std::optional<location> arg_location = std::nullopt
				)
			: ast_expression{get_rtti_index(), loc},
			  generics_{std::move(generics)},
			  generic_packs_{std::move(generic_packs)},
			  self_{self},
			  args_{std::move(args)},
			  has_vararg_{vararg.has_value()},
			  vararg_loc_{vararg.value_or(location{})},
			  vararg_annotation_{vararg_annotation},
			  body_{body},
			  function_depth_{function_depth},
			  debug_name_{std::move(debug_name)},
			  has_return_annotation_{return_annotation.has_value()},
			  return_annotation_{return_annotation.has_value() ? std::move(*return_annotation) : ast_type_list{}},
			  has_end_{has_end},
			  arg_location_{arg_location} {}

		void visit(ast_visitor& visitor) override
		{
			if (visitor.visit(*this))
			{
				for (const auto& arg: args_)
				{
					if (const auto& annotation = arg->annotation;
						annotation) { annotation->visit(visitor); }
				}

				if (vararg_annotation_) { vararg_annotation_->visit(visitor); }

				if (has_return_annotation_) { visitor.visit(return_annotation_); }

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
				list,
				// value only
				record,
				// key is a ast_expression_constant_string
				general,
			};

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
}

#endif // GAL_LANG_AST_AST_HPP
