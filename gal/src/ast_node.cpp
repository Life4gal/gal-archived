#include "ast_node.hpp"

#include <typeinfo>

namespace gal
{
	ast_expression::~ast_expression() = default;

	std::string ast_integer::to_string() const noexcept
	{
		return std::string{"ast_integer("} + typeid(value_).name() + "): " + std::to_string(value_);
	}

	std::string ast_double::to_string() const noexcept
	{
		return std::string{"ast_double("} + typeid(value_).name() + "): " + std::to_string(value_);
	}

	std::string ast_string::to_string() const noexcept
	{
		return std::string{"ast_string("} + typeid(value_).name() + "): " + value_;
	}

	std::string ast_boolean::to_string() const noexcept
	{
		return std::string{"ast_boolean("} + typeid(value_).name() + "): " + std::to_string(value_);
	}
}
