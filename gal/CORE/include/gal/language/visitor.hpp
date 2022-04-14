#pragma once

#ifndef GAL_LANG_LANGUAGE_VISITOR_HPP
#define GAL_LANG_LANGUAGE_VISITOR_HPP

#include <gal/language/common.hpp>
#include <iostream>

namespace gal::lang::lang
{
	namespace visitor_detail
	{
		template<typename... Visitors>
		class default_visitor final : public ast_visitor, private Visitors...
		{
		public:
			constexpr default_visitor() noexcept((std::is_nothrow_default_constructible_v<Visitors> && ...)) requires(std::is_default_constructible_v<Visitors> && ...) = default;

			constexpr explicit default_visitor(Visitors&&... visitors)
				: Visitors{std::forward<Visitors>(visitors)}... {}

			void visit(const ast_node& node) override { (static_cast<Visitors&>(*this)(node), ...); }
		};

		struct print_visitor
		{
			void operator()(const ast_node& node) const { std::clog << std_format::format("visiting node: \n{}\n", node.to_string()); }
		};

		// todo: more visitor
	}

	using default_visitor = visitor_detail::default_visitor<
		// visitor_detail::print_visitor
	>;
}

#endif // GAL_LANG_LANGUAGE_VISITOR_HPP
