#pragma once

#ifndef GAL_LANG_ADDON_AST_VISITOR_HPP
#define GAL_LANG_ADDON_AST_VISITOR_HPP

#include <gal/foundation/ast.hpp>
#include <gal/tools/logger.hpp>
#include <gal/defines.hpp>

namespace gal::lang::addon
{
	namespace visitor_detail
	{
		template<typename... Visitors>
		class ast_visitor final : public ast::ast_visitor_base, private Visitors...
		{
		public:
			constexpr ast_visitor() noexcept((std::is_nothrow_default_constructible_v<Visitors> && ...)) requires(std::is_default_constructible_v<Visitors> && ...) = default;

			constexpr explicit ast_visitor(Visitors&&... visitors)
				: Visitors{std::forward<Visitors>(visitors)}... {}

			void visit(const ast::ast_node& node) override { (static_cast<Visitors&>(*this)(node), ...); }
		};

		struct print_visitor
		{
			void operator()([[maybe_unused]] const ast::ast_node& node) const
			{
				GAL_LANG_AST_VISIT_PRINT_DO(
						tools::logger::info(
							"\n=====print_visitor starts printing ast_node====\n"
							"{}\n"
							"=====print_visitor ends printing ast_node====\n", node.to_string());)
			}
		};

		// todo: more visitor
	}

	using ast_visitor = visitor_detail::ast_visitor<
		visitor_detail::print_visitor
	>;
}

#endif // GAL_LANG_ADDON_AST_VISITOR_HPP
