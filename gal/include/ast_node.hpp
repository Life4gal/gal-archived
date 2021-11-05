#pragma once

#ifndef GAL_LANG_AST_NODE_HPP
	#define GAL_LANG_AST_NODE_HPP

	#include <cstdint>
	#include <string>

namespace gal
{
	using integer_type = std::int64_t;
	using number_type  = double;
	using string_type = std::string;
	using boolean_type = std::uint_fast8_t;

	using identifier_type = std::string;

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
	};

	class ast_expression;
	class ast_statement;
	class ast_integer;
	class ast_number;
	class ast_string;
	class ast_boolean;
	class ast_identifier;

	/*! Represents an expression. */
	class ast_expression
	{
	public:
		ast_expression()						  = default;

		virtual ~ast_expression()				  = 0;
		ast_expression(const ast_expression&)	  = default;
		ast_expression(ast_expression&&)		  = default;
		ast_expression&					  operator=(const ast_expression&) = default;
		ast_expression&					  operator=(ast_expression&&) = default;

		/*! Returns the type of the node. */
		constexpr virtual ast_expression_type get_type()				  = 0;

		/*! Returns the textual representation. */
		[[nodiscard]] virtual std::string to_string() const noexcept { return "ast_expression"; }
	};

	/*! Represents a statement. */
	class ast_statement : public ast_expression
	{
	public:
		constexpr ast_expression_type get_type() override { return ast_expression_type::statement_t; }
		[[nodiscard]] std::string to_string() const noexcept override { return "ast_statement"; }
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
		[[nodiscard]] std::string to_string() const noexcept override;
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
		[[nodiscard]] std::string to_string() const noexcept override;
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

		constexpr ast_expression_type get_type() override { return ast_expression_type::string_t; }
		[[nodiscard]] std::string to_string() const noexcept override;
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
		[[nodiscard]] std::string to_string() const noexcept override;
	};

	/*! Represents an identifier. */
	class ast_identifier final : public ast_expression
	{
	public:
		using value_type = identifier_type;

	private:
		value_type name_;

	public:
		explicit ast_identifier(value_type name) : name_(std::move(name)) {}

		constexpr ast_expression_type get_type() override { return ast_expression_type::identifier_t; }
		[[nodiscard]] std::string to_string() const noexcept override;
	};
}// namespace gal

#endif//GAL_LANG_AST_NODE_HPP
