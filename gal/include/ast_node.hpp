#pragma once

#ifndef GAL_LANG_AST_NODE_HPP
#define GAL_LANG_AST_NODE_HPP

#include <string>
#include <cstdint>

namespace yy
{
	struct location;
}

namespace gal
{
	/*! Type of the AST node */
	enum class node_type
	{
		// todo: fill it!
		node_expression,
		node_integer,
		node_number,
		node_string,
		node_boolean,
	};

	class ast_node
	{
	public:
		ast_node() = default;

		virtual ~ast_node() = 0;
		ast_node(const ast_node&) = default;
		ast_node(ast_node&&) = default;
		ast_node& operator=(const ast_node&) = default;
		ast_node& operator=(ast_node&&) = default;

		/*! Returns the type of the node. */
		constexpr virtual node_type get_type() = 0;

		/*! Returns the textual representation. */
		virtual std::string to_string() { return "ast_node"; }
	};

	/*! Represents an expression. */
	class ast_expression : public ast_node
	{
	public:
		std::string to_string() override { return "ast_expression"; }
	};

	/*! Represents a statement. */
	class ast_statement : public ast_expression
	{
	public:
		constexpr node_type get_type() override { return node_type::node_expression; }
		std::string to_string() override { return "ast_statement"; }
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

		constexpr node_type get_type() override { return node_type::node_integer; }
		std::string to_string() override;
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

		constexpr node_type get_type() override { return node_type::node_number; }
		std::string to_string() override;
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

		constexpr node_type get_type() override { return node_type::node_string; }
		std::string to_string() override;
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

		constexpr node_type get_type() override { return node_type::node_boolean; }
		std::string to_string() override;
	};
};

#endif//GAL_LANG_AST_NODE_HPP
