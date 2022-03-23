#pragma once

#ifndef GAL_LANG_LANGUAGE_OPTIMIZER_HPP
#define GAL_LANG_LANGUAGE_OPTIMIZER_HPP

#include <gal/language/eval.hpp>

namespace gal::lang::lang
{
	namespace optimizer_detail
	{
		template<typename... Optimizers>
		struct optimizer : Optimizers...
		{
			optimizer() requires(std::is_default_constructible_v<Optimizers> && ...) = default;

			explicit optimizer(Optimizers ... optimizers)
				: Optimizers{std::move(optimizers)}... {}

			template<typename Tracer>
			auto optimize(ast_node_ptr<Tracer> p)
			{
				((p = static_cast<Optimizers&>(*this)(std::move(p))), ...);
				return p;
			}
		};

		template<typename Tracer>
		[[nodiscard]] bool node_empty(const ast_node<Tracer>& node) noexcept
		{
			if (node.template is<compiled_ast_node<Tracer>>()) { return dynamic_cast<const compiled_ast_node<Tracer>&>(node).original_node->empty(); }
			return node.empty();
		}

		template<typename Tracer>
		[[nodiscard]] auto node_size(const ast_node<Tracer>& node) noexcept
		{
			if (node.template is<compiled_ast_node<Tracer>>()) { return dynamic_cast<const compiled_ast_node<Tracer>&>(node).original_node->size(); }
			return node.size();
		}

		template<typename Tracer>
		[[nodiscard]] decltype(auto) node_child(ast_node<Tracer>& node, typename ast_node<Tracer>::children_type::size_type offset) noexcept
		{
			gal_assert(offset < node_size(node));
			if (auto& child = node.get_child(offset);
				child.template is<compiled_ast_node<Tracer>>()) { return *dynamic_cast<compiled_ast_node<Tracer>&>(child).original_node; }
			else { return child; }
		}

		template<typename Tracer>
		[[nodiscard]] decltype(auto) node_child(const ast_node<Tracer>& node, typename ast_node<Tracer>::children_type::size_type offset) noexcept
		{
			gal_assert(offset < node_size(node));
			if (auto& child = node.get_child(offset);
				child.template is<compiled_ast_node<Tracer>>()) { return *dynamic_cast<compiled_ast_node<Tracer>&>(child).original_node; }
			else { return child; }
		}

		template<typename Tracer>
		[[nodiscard]] bool node_has_var_decl(const ast_node<Tracer>& node) noexcept
		{
			if (node.template is_any<var_decl_ast_node<Tracer>, assign_decl_ast_node<Tracer>, reference_ast_node<Tracer>>()) { return true; }

			return std::ranges::any_of(
					node,
					[](const auto& child)
					{
						return not child.template is_any<block_ast_node<Tracer>, for_ast_node<Tracer>, ranged_for_ast_node<Tracer>>() &&
						       node_has_var_decl(child);
					});
		}

		struct return_optimizer
		{
			template<typename T>
			ast_node_ptr<T> operator()(ast_node_ptr<T> p)
			{
				if (p->template is_any<def_ast_node<T>, lambda_ast_node<T>>() && not p->empty())
				{
					if (auto& back = p->back();
						back.template is<block_ast_node<T>>())
					{
						if (auto& block_back = back.back();
							block_back.template is<return_ast_node<T>>()) { if (block_back.size() == 1) { block_back = std::move(block_back.front()); } }
					}
				}

				return p;
			}
		};

		struct block_optimizer
		{
			template<typename T>
			ast_node_ptr<T> operator()(ast_node_ptr<T> p)
			{
				if (p->template is<block_ast_node<T>>())
				{
					if (not node_has_var_decl(*p))
					{
						if (p->size() == 1) { return std::move(p->front()); }

						return std::move(*p).template remake_node<no_scope_block_ast_node<T>>();
					}
				}

				return p;
			}
		};

		struct dead_code_optimizer
		{
			template<typename T>
			ast_node_ptr<T> operator()(ast_node_ptr<T> p)
			{
				if (p->template is<block_ast_node<T>>())
				{
					typename ast_node<T>::children_type children{};
					p->swap(children);

					children.erase(std::ranges::remove_if(
							children,
							[](const auto& child) { return child.template is_any<noop_ast_node<T>, id_ast_node<T>, constant_ast_node<T>>(); }));

					p->swap(children);

					return std::move(*p).template remake_node<block_ast_node<T>>();
				}

				return p;
			}
		};

		// todo: more optimizer
	}

	using default_optimizer = optimizer_detail::optimizer<
		optimizer_detail::return_optimizer,
		optimizer_detail::block_optimizer,
		optimizer_detail::dead_code_optimizer>;
}

#endif // GAL_LANG_LANGUAGE_OPTIMIZER_HPP
