#pragma once

#ifndef GAL_LANG_AST_NODE_HPP
#define GAL_LANG_AST_NODE_HPP

#include <string>
#include <cstdint>

namespace gal
{
	/*! Type of the AST node */
	enum class ast_type
	{
		// todo: fill it!
		expression_t,
		integer_t,
		number_t,
		string_t,
		boolean_t,
	};

	/*! Represents an expression. */
	class ast_expression
	{
	public:
		ast_expression() = default;

		virtual ~ast_expression() = 0;
		ast_expression(const ast_expression&) = default;
		ast_expression(ast_expression&&) = default;
		ast_expression& operator=(const ast_expression&) = default;
		ast_expression& operator=(ast_expression&&) = default;

		/*! Returns the type of the node. */
		constexpr virtual ast_type		  get_type() = 0;

		/*! Returns the textual representation. */
		[[nodiscard]] virtual std::string to_string() const noexcept { return "ast_expression"; }
	};

	/*! Represents a statement. */
	class ast_statement : public ast_expression
	{
	public:
		constexpr ast_type		  get_type() override { return ast_type::expression_t; }
		[[nodiscard]] std::string to_string() const noexcept override { return "ast_statement"; }
	};

	/*! Represents an integer. */
	class ast_integer final : public ast_expression
	{
	public:
		using value_type = std::int64_t;

	private:
		value_type value_;

	public:
		constexpr explicit ast_integer(value_type value) : value_(value) {}

		constexpr ast_type		  get_type() override { return ast_type::integer_t; }
		[[nodiscard]] std::string to_string() const noexcept override;
	};

	/*! Represents a double. */
	class ast_double final : public ast_expression
	{
	public:
		using value_type = long double;

	private:
		value_type value_;

	public:
		constexpr explicit ast_double(value_type value) : value_(value) {}

		constexpr ast_type		  get_type() override { return ast_type::number_t; }
		[[nodiscard]] std::string to_string() const noexcept override;
	};

	/*! Represents a string. */
	class ast_string final : public ast_expression
	{
	public:
		using value_type = std::string;

	private:
		value_type value_;

	public:
		explicit ast_string(value_type value) : value_(std::move(value)) {}

		constexpr ast_type		  get_type() override { return ast_type::string_t; }
		[[nodiscard]] std::string to_string() const noexcept override;
	};

	/*! Represents a boolean. */
	class ast_boolean final : public ast_expression
	{
	public:
		using value_type = std::uint_fast8_t;

	private:
		value_type value_;

	public:
		constexpr explicit ast_boolean(value_type value) : value_(value) {}

		constexpr ast_type		  get_type() override { return ast_type::boolean_t; }
		[[nodiscard]] std::string to_string() const noexcept override;
	};
}

#endif//GAL_LANG_AST_NODE_HPP
