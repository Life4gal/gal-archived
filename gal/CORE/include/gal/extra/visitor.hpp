#pragma once

#ifndef GAL_LANG_EXTRA_VISITOR_HPP
#define GAL_LANG_EXTRA_VISITOR_HPP

#include <gal/language/common.hpp>
#include <gal/tools/logger.hpp>
#include <gal/defines.hpp>

namespace gal::lang::extra
{
	namespace visitor_detail
	{
		template<typename... Visitors>
		class default_visitor final : public lang::ast_visitor, private Visitors...
		{
		public:
			constexpr default_visitor() noexcept((std::is_nothrow_default_constructible_v<Visitors> && ...)) requires(std::is_default_constructible_v<Visitors> && ...) = default;

			constexpr explicit default_visitor(Visitors&&... visitors)
				: Visitors{std::forward<Visitors>(visitors)}... {}

			void visit(const lang::ast_node& node) override { (static_cast<Visitors&>(*this)(node), ...); }
		};

		struct print_visitor
		{
			void operator()([[maybe_unused]] const lang::ast_node& node) const
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

	using default_visitor = visitor_detail::default_visitor<
		visitor_detail::print_visitor
	>;
}

#endif // GAL_LANG_EXTRA_VISITOR_HPP
