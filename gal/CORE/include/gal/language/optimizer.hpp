#pragma once

#ifndef GAL_LANG_LANGUAGE_OPTIMIZER_HPP
#define GAL_LANG_LANGUAGE_OPTIMIZER_HPP

#include <gal/language/eval.hpp>

namespace gal::lang::lang
{
	namespace optimizer_detail
	{
		template<typename... Optimizers>
		class default_optimizer final : public ast_optimizer, private Optimizers...
		{
			constexpr default_optimizer() noexcept((std::is_nothrow_default_constructible_v<Optimizers> && ...)) requires(std::is_default_constructible_v<Optimizers> && ...) = default;

			constexpr explicit default_optimizer(Optimizers&&... optimizers)
				: Optimizers{std::forward<Optimizers>(optimizers)}... {}

			[[nodiscard]] ast_node_ptr optimize(ast_node_ptr node) override
			{
				((node = static_cast<Optimizers&>(*this)(std::move(node))), ...);
				return node;
			}
		};

		[[nodiscard]] inline bool node_empty(const ast_node& node) noexcept
		{
			if (node.is<compiled_ast_node>()) { return dynamic_cast<const compiled_ast_node&>(node).original_node->empty(); }
			return node.empty();
		}

		[[nodiscard]] inline auto node_size(const ast_node& node) noexcept
		{
			if (node.is<compiled_ast_node>()) { return dynamic_cast<const compiled_ast_node&>(node).original_node->size(); }
			return node.size();
		}

		[[nodiscard]] inline ast_node& node_child(ast_node& node, const ast_node::children_type::difference_type offset) noexcept
		{
			gal_assert(offset < static_cast<ast_node::children_type::difference_type>(node_size(node)));
			if (auto& child = node.get_child(offset);
				child.is<compiled_ast_node>()) { return *dynamic_cast<compiled_ast_node&>(child).original_node; }
			else { return child; }
		}

		[[nodiscard]] inline const ast_node& node_child(const ast_node& node, const ast_node::children_type::difference_type offset) noexcept
		{
			gal_assert(offset < static_cast<ast_node::children_type::difference_type>(node_size(node)));
			if (auto& child = node.get_child(offset);
				child.is<compiled_ast_node>()) { return *dynamic_cast<const compiled_ast_node&>(child).original_node; }
			else { return child; }
		}

		[[nodiscard]] inline bool node_has_var_decl(const ast_node& node) noexcept
		{
			if (node.is_any<var_decl_ast_node, assign_decl_ast_node, reference_ast_node>()) { return true; }

			return std::ranges::any_of(
					node,
					[](const auto& child)
					{
						return not child.template is_any<block_ast_node, for_ast_node, ranged_for_ast_node>() &&
						       node_has_var_decl(child);
					});
		}

		struct return_optimizer
		{
			ast_node_ptr operator()(ast_node_ptr p) const
			{
				if (p->is_any<def_ast_node, lambda_ast_node>() && not p->empty())
				{
					if (auto& back = p->back();
						back.is<block_ast_node>())
					{
						if (auto& block_back = back.back();
							block_back.is<return_ast_node>()) { if (block_back.size() == 1) { block_back = std::move(block_back.front()); } }
					}
				}

				return p;
			}
		};

		struct block_optimizer
		{
			ast_node_ptr operator()(ast_node_ptr p) const
			{
				if (p->is<block_ast_node>())
				{
					if (not node_has_var_decl(*p))
					{
						if (p->size() == 1) { return std::move(p->get_child_ptr(0)); }

						return std::move(*p).remake_node<no_scope_block_ast_node>();
					}
				}

				return p;
			}
		};

		struct dead_code_optimizer
		{
			ast_node_ptr operator()(ast_node_ptr p) const
			{
				if (p->is<block_ast_node>())
				{
					ast_node::children_type children{};
					p->swap(children);

					const auto view = std::ranges::remove_if(
							children,
							[](const auto& child) { return child->template is_any<noop_ast_node, id_ast_node, constant_ast_node>(); });
					children.erase(view.begin(), view.end());

					p->swap(children);

					return std::move(*p).remake_node<block_ast_node>();
				}

				return p;
			}
		};

		// todo: more optimizer
	}// namespace optimizer_detail
	using default_optimizer = optimizer_detail::default_optimizer<
		optimizer_detail::return_optimizer,
		optimizer_detail::block_optimizer,
		optimizer_detail::dead_code_optimizer
	>;
};

#endif // GAL_LANG_LANGUAGE_OPTIMIZER_HPP
