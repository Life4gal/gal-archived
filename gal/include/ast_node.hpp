#pragma once

#ifndef GAL_LANG_AST_NODE_HPP
	#define GAL_LANG_AST_NODE_HPP

	#include <cstdint>
	#include <string>
	#include <map>
	#include <memory>
	#include <optional>
	#include <set>
	#include <string_view>
	#include <variant>
	#include <vector>

namespace gal
{
	/*! Type of the AST node */
	enum class ast_expression_type
	{
		// todo: fill it!
		statement_t,

		integer_t,
		number_t,
		string_t,
		boolean_t,

		identifier_t,
		args_pack_t,
		prototype_t,
		function_t,
		if_branch_t,
		else_branch_t,
	};

	class ast_expression;
	class ast_statement;
	class ast_integer;
	class ast_number;
	class ast_string;
	class ast_boolean;
	class ast_identifier;
	class context_scope;
	class ast_args_pack;
	class ast_prototype;
	class ast_function;
	class ast_else_expr;
	class ast_if_expr;

	template<typename T, typename... Args>
	std::unique_ptr<ast_expression> make_expression(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	/*! Represents an integer value. */
	using integer_type		   = std::int64_t;
	/*! Represents a number value. */
	using number_type		   = double;
	/*! Represents a string value. */
	using string_type		   = std::string;
	/*! Represents a boolean value. */
	using boolean_type		   = std::uint_fast8_t;

	/*! Represents an identifier. */
	using identifier_type	   = std::string;
	/*! Represents an identifier view. */
	using identifier_view_type = std::string_view;
	/*! Represents an expression. */
	using expression_type	   = std::unique_ptr<ast_expression>;

	/*! Represents an expression. */
	class ast_expression
	{
	public:
		ast_expression()							  = default;

		virtual ~ast_expression()					  = 0;
		ast_expression(const ast_expression&)		  = default;
		ast_expression(ast_expression&&)			  = default;
		ast_expression&						  operator=(const ast_expression&) = default;
		ast_expression&						  operator=(ast_expression&&) = default;

		/*! Returns the type of the node. */
		constexpr virtual ast_expression_type get_type()				  = 0;

		/*! Returns the textual representation. */
		[[nodiscard]] virtual std::string	  to_string() const noexcept { return "ast_expression"; }
	};

	/*! Represents a statement. */
	class ast_statement : public ast_expression
	{
	public:
		constexpr ast_expression_type get_type() override { return ast_expression_type::statement_t; }
		[[nodiscard]] std::string	  to_string() const noexcept override { return "ast_statement"; }
	};

	/*! Represents an integer. */
	class ast_integer final : public ast_expression
	{
	public:
		using value_type = integer_type;

	private:
		value_type value_;

	public:
		constexpr explicit ast_integer(value_type value) : value_(value) {}

		constexpr ast_expression_type get_type() override { return ast_expression_type::integer_t; }
		[[nodiscard]] std::string	  to_string() const noexcept override;

		[[nodiscard]] value_type	  get_value() const noexcept
		{
			return value_;
		}

		void set_value(value_type value) noexcept
		{
			value_ = value;
		}
	};

	/*! Represents a double. */
	class ast_number final : public ast_expression
	{
	public:
		using value_type = number_type;

	private:
		value_type value_;

	public:
		constexpr explicit ast_number(value_type value) : value_(value) {}

		constexpr ast_expression_type get_type() override { return ast_expression_type::number_t; }
		[[nodiscard]] std::string	  to_string() const noexcept override;

		[[nodiscard]] value_type	  get_value() const noexcept
		{
			return value_;
		}

		void set_value(value_type value) noexcept
		{
			value_ = value;
		}
	};

	/*! Represents a string. */
	class ast_string final : public ast_expression
	{
	public:
		using value_type = string_type;

	private:
		value_type value_;

	public:
		explicit ast_string(value_type value) : value_(std::move(value)) {}

		constexpr ast_expression_type	get_type() override { return ast_expression_type::string_t; }
		[[nodiscard]] std::string		to_string() const noexcept override;

		[[nodiscard]] const value_type& get_value() const& noexcept
		{
			return value_;
		}

		[[nodiscard]] value_type&& get_value() && noexcept
		{
			return std::move(value_);
		}

		void set_value(value_type value) noexcept
		{
			value_ = std::move(value);
		}
	};

	/*! Represents a boolean. */
	class ast_boolean final : public ast_expression
	{
	public:
		using value_type = boolean_type;

	private:
		value_type value_;

	public:
		constexpr explicit ast_boolean(value_type value) : value_(value) {}

		constexpr ast_expression_type get_type() override { return ast_expression_type::boolean_t; }
		[[nodiscard]] std::string	  to_string() const noexcept override;

		constexpr explicit operator bool() const noexcept
		{
			return bool(value_);
		}

		constexpr bool flip() noexcept
		{
			return std::exchange(value_, !value_);
		}
	};

	/*! Represents an identifier. */
	class ast_identifier final : public ast_expression
	{
	public:
		using value_type = identifier_type;
		using view_type = identifier_view_type;

	private:
		value_type name_;

	public:
		explicit ast_identifier(value_type name) : name_(std::move(name)) {}

		constexpr ast_expression_type get_type() override { return ast_expression_type::identifier_t; }
		[[nodiscard]] std::string	  to_string() const noexcept override;

		[[nodiscard]] const value_type&			  get_name() const noexcept
		{
			return name_;
		}
	};

	/*! Represents a args_pack. */
	class ast_args_pack final : public ast_expression
	{
	public:
		friend class context;

		using value_type	 = identifier_type;
		using args_pack_type = std::vector<value_type>;

	private:
		args_pack_type args_;

	public:
		explicit ast_args_pack(args_pack_type args = {}) : args_(std::move(args)) {}

		constexpr ast_expression_type get_type() override { return ast_expression_type::args_pack_t; }
		[[nodiscard]] std::string	  to_string() const noexcept override;

		/*! Push more args if needed. */
		decltype(auto)				  push_arg(value_type arg)
		{
			return args_.emplace_back(std::move(arg));
		}
	};

	/*! Represents a prototype for a function. */
	class ast_prototype final : public ast_expression
	{
	public:
		friend class context;

	private:
		identifier_type name_;
		ast_args_pack	args_;

	public:
		explicit ast_prototype(identifier_type name, ast_args_pack args = ast_args_pack{})
			: name_(std::move(name)),
			  args_(std::move(args)) {}

		constexpr ast_expression_type		 get_type() override { return ast_expression_type::prototype_t; }
		[[nodiscard]] std::string			 to_string() const noexcept override;

		[[nodiscard]] const identifier_type& get_prototype_name() const noexcept
		{
			return name_;
		}

		/*! Push more args if needed. */
		decltype(auto) push_arg(ast_args_pack::value_type&& arg)
		{
			return args_.push_arg(std::forward<ast_args_pack::value_type>(arg));
		}
	};

	/*! Represents a function. */
	class ast_function final : public ast_expression
	{
	public:
		friend class context;

		using prototype_type = std::unique_ptr<ast_prototype>;

	private:
		prototype_type	prototype_;
		expression_type body_;

	public:
		ast_function(prototype_type prototype, expression_type body)
			: prototype_(std::move(prototype)),
			  body_(std::move(body)) {}

		constexpr ast_expression_type get_type() override { return ast_expression_type::function_t; }
		[[nodiscard]] std::string	  to_string() const noexcept override;
	};

	/*! Represents a else branch. */
	class ast_else_expr final : public ast_expression
	{
	public:
		friend class context;

	private:
		expression_type body_;

	public:
		explicit ast_else_expr(expression_type body) : body_(std::move(body)) {}

		constexpr ast_expression_type get_type() override { return ast_expression_type::else_branch_t; }
		[[nodiscard]] std::string	  to_string() const noexcept override;
	};

	/*! Represents a if-expression. */
	class ast_if_expr final : public ast_expression
	{
	public:
		friend class context;

		using chain_type = std::vector<expression_type>;

	private:
		expression_type condition_;
		expression_type branch_then_;

		/*! ast_if_expr/ast_else_expr */
		chain_type		branch_chain_;

		explicit ast_if_expr(expression_type branch_then, expression_type condition)
			: condition_(std::move(condition)),
			  branch_then_(std::move(branch_then)) {}

		constexpr ast_expression_type get_type() override { return ast_expression_type::if_branch_t; }
		[[nodiscard]] std::string	  to_string() const noexcept override;

		void						  add_branch(expression_type branch)
		{
			branch_chain_.push_back(std::move(branch));
		}
	};
}// namespace gal

#endif//GAL_LANG_AST_NODE_HPP
