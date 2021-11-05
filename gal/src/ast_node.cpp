#include "ast_node.hpp"

#include <typeinfo>
#include <utility>
#include <ranges>

namespace gal
{
	ast_expression::~ast_expression() = default;

	std::string ast_integer::to_string() const noexcept
	{
		return std::string{"ast_integer("} + typeid(value_).name() + "): " + std::to_string(value_);
	}

	std::string ast_number::to_string() const noexcept
	{
		return std::string{"ast_number("} + typeid(value_).name() + "): " + std::to_string(value_);
	}

	std::string ast_string::to_string() const noexcept
	{
		return std::string{"ast_string("} + typeid(value_).name() + "): " + value_;
	}

	std::string ast_boolean::to_string() const noexcept
	{
		return std::string{"ast_boolean("} + typeid(value_).name() + "): " + std::to_string(value_);
	}

	std::string ast_identifier::to_string() const noexcept
	{
		return std::string{"ast_identifier: "} + name_;
	}

	std::string ast_scope::to_string() const noexcept
	{
		return std::string{"ast_scope: "} + name_;
	}

	std::string ast_args_pack::to_string() const noexcept
	{
		return std::string{"ast_args_pack: "} + " with " + std::to_string(args_.size()) + "args";
	}

	std::string ast_prototype::to_string() const noexcept
	{
		return std::string{"ast_prototype: "} + name_ + " -> " + args_.to_string();
	}

	std::string ast_function::to_string() const noexcept
	{
		return std::string{"ast_function: "} + prototype_->to_string() + " -> " + body_->to_string();
	}

	std::string ast_else_expr::to_string() const noexcept
	{
		return std::string{"ast_else_expr: "} + body_->to_string();
	}

	std::string ast_if_expr::to_string() const noexcept
	{
		return std::string{"ast_if_expr: "} + (condition_ ? condition_->to_string() : "no condition") + " -> " + branch_then_->to_string();
	}
}
