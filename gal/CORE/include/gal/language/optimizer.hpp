#pragma once

#ifndef GAL_LANG_KITS_OPTIMIZER_HPP
#define GAL_LANG_KITS_OPTIMIZER_HPP

#include <gal/language/eval.hpp>

namespace gal::lang
{
	namespace optimize
	{
		template<typename... Optimizers>
		struct optimizer : Optimizers...
		{
			optimizer() requires(std::is_default_constructible_v<Optimizers> && ...) = default;

			explicit optimizer(Optimizers ... optimizers)
				: Optimizers{std::move(optimizers)}... {}

			template<typename Tracer>
			auto optimize(eval::ast_node_impl_ptr<Tracer> p)
			{
				((p = static_cast<Optimizers&>(*this)(std::move(p))), ...);
				return p;
			}
		};

		template<typename T>
		struct return_optimizer
		{
			eval::ast_node_impl_ptr<T> operator()(eval::ast_node_impl_ptr<T> p)
			{
				if (utils::is_any_enum_of(p->type, ast_node_type::def_t, ast_node_type::lambda_t) && p->empty())
				{
					if (auto& back = p->back();
						back.type == ast_node_type::block_t)
					{
						if (auto& block_back = back.back();
							block_back.type == ast_node_type::return_t) { if (block_back.size() == 1) { block_back = std::move(block_back.front()); } }
					}
				}

				return p;
			}
		};
	}
}

#endif // GAL_LANG_KITS_OPTIMIZER_HPP
