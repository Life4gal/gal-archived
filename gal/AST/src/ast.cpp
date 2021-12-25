#include <ast.hpp>

namespace gal
{
	void ast_local::visit(ast_visitor& visitor) const { if (annotation) { annotation->visit(visitor); } }

	void ast_type_list::visit(ast_visitor& visitor) const
	{
		for (const auto& type: types) { type->visit(visitor); }

		if (tail_type) { tail_type->visit(visitor); }
	}

	constexpr ast_visitor::~ast_visitor() = default;

	ast_name ast_expression::get_identifier() const noexcept
	{
	
		if (const auto* expression = this->as<ast_expression_global>(); expression)
		{
			return expression->get_name();
		}

		if (const auto* expression = this->as<ast_expression_local>(); expression)
		{
			return expression->get_local()->get_name();
		}

		return ast_name{};
	}
}
