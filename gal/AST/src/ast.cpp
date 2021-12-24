#include <ast.hpp>

namespace gal
{
	constexpr ast_visitor::~ast_visitor() = default;

	void ast_visitor::visit(const ast_type_list& list)
	{
		const auto& [types, tail_type] = list;

		for (const auto& type : types)
		{
			type->visit(*this);
		}

		if (tail_type)
		{
			tail_type->visit(*this);
		}
	}

}
