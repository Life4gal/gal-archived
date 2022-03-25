#pragma once

#ifndef GAL_LANG_LANGUAGE_VISITOR_HPP
#define GAL_LANG_LANGUAGE_VISITOR_HPP

#include <gal/language/common.hpp>

namespace gal::lang::lang
{
	class default_visitor final : public ast_visitor
	{
	public:
		void visit(const ast_node& node) override
		{
			// todo
			(void)node;
		}
	};
}

#endif // GAL_LANG_LANGUAGE_VISITOR_HPP
