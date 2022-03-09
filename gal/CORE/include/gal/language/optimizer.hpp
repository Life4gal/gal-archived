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

		template<typename Tracer>
		[[nodiscard]] bool node_empty(const eval::ast_node_impl_base<Tracer>& node) noexcept
		{
			if (node.typeast_node_type::compiled_t) { return dynamic_cast<const eval::compiled_ast_node<Tracer>&>(node).original_node->empty(); }
			return node.empty();
		}

		template<typename Tracer>
		[[nodiscard]] auto node_size(const eval::ast_node_impl_base<Tracer>& node) noexcept
		{
			if (node.typeast_node_type::compiled_t) { return dynamic_cast<const eval::compiled_ast_node<Tracer>&>(node).original_node->size(); }
			return node.size();
		}

		template<typename Tracer>
		[[nodiscard]] decltype(auto) node_child(eval::ast_node_impl_base<Tracer>& node, typename eval::ast_node_impl_base<Tracer>::children_type::size_type offset) noexcept
		{
			gal_assert(offset < node_size(node));
			if (auto& child = node.get_child(offset);
				child.type == ast_node_type::compiled_t) { return *dynamic_cast<eval::compiled_ast_node<Tracer>&>(child).original_node; }
			else { return child; }
		}

		template<typename Tracer>
		[[nodiscard]] decltype(auto) node_child(const eval::ast_node_impl_base<Tracer>& node, typename eval::ast_node_impl_base<Tracer>::children_type::size_type offset) noexcept
		{
			gal_assert(offset < node_size(node));
			if (auto& child = node.get_child(offset);
				child.type == ast_node_type::compiled_t) { return *dynamic_cast<eval::compiled_ast_node<Tracer>&>(child).original_node; }
			else { return child; }
		}

		template<typename Tracer>
		[[nodiscard]] bool node_has_var_decl(const eval::ast_node_impl_base<Tracer>& node) noexcept
		{
			if (utils::is_any_enum_of(node.type, ast_node_type::var_decl_t, ast_node_type::assign_decl_t, ast_node_type::reference_t)) { return true; }

			return std::ranges::any_of(
					node,
					[](const auto& child)
					{
						return not utils::is_any_enum_of(child.type, ast_node_type::block_t, ast_node_type::for_t, ast_node_type::ranged_for_t) &&
						       node_has_var_decl(child);
					}
					);
		}

		struct return_optimizer
		{
			template<typename T>
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

		struct block_optimizer
		{
			template<typename T>
			eval::ast_node_impl_ptr<T> operator()(eval::ast_node_impl_ptr<T> p)
			{
				if (p->type == ast_node_type::block_t)
				{
					if (not node_has_var_decl(*p))
					{
						if (p->size() == 1) { return std::move(p->front()); }

						return eval::remake_node<eval::no_scope_block_ast_node<T>>(*p);
					}
				}

				return p;
			}
		};

		struct dead_code_optimizer
		{
			template<typename T>
			eval::ast_node_impl_ptr<T> operator()(eval::ast_node_impl_ptr<T> p)
			{
				if (p->type == ast_node_type::block_t)
				{
					typename eval::ast_node_impl_base<T>::children_type children{};
					p->swap(children);

					children.erase(std::ranges::remove_if(
							children,
							[](const auto& child) { return utils::is_any_enum_of(child.type, ast_node_type::id_t, ast_node_type::constant_t, ast_node_type::noop_t); }));

					p->swap(children);

					return eval::remake_node<eval::block_ast_node<T>>(*p);
				}

				return p;
			}
		};
	}
}

#endif // GAL_LANG_KITS_OPTIMIZER_HPP
