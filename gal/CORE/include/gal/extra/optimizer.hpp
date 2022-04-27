#pragma once

#ifndef GAL_LANG_EXTRA_OPTIMIZER_HPP
#define GAL_LANG_EXTRA_OPTIMIZER_HPP

#include <gal/language/eval.hpp>
#include <vector>

namespace gal::lang::extra
{
	namespace optimizer_detail
	{
		template<typename... Optimizers>
		class default_optimizer final : public lang::ast_optimizer, private Optimizers...
		{
		public:
			constexpr default_optimizer() noexcept((std::is_nothrow_default_constructible_v<Optimizers> && ...)) requires(std::is_default_constructible_v<Optimizers> && ...) = default;

			constexpr explicit default_optimizer(Optimizers&&... optimizers)
				: Optimizers{std::forward<Optimizers>(optimizers)}... {}

			[[nodiscard]] lang::ast_node_ptr optimize(lang::ast_node_ptr node) override
			{
				((node = static_cast<Optimizers&>(*this)(std::move(node))), ...);
				return node;
			}
		};

		[[nodiscard]] inline bool node_empty(const lang::ast_node& node) noexcept
		{
			if (node.is<lang::compiled_ast_node>()) { return dynamic_cast<const lang::compiled_ast_node&>(node).original_node->empty(); }
			return node.empty();
		}

		[[nodiscard]] inline auto node_size(const lang::ast_node& node) noexcept
		{
			if (node.is<lang::compiled_ast_node>()) { return dynamic_cast<const lang::compiled_ast_node&>(node).original_node->size(); }
			return node.size();
		}

		[[nodiscard]] inline lang::ast_node& node_child(lang::ast_node& node, const lang::ast_node::children_type::difference_type offset) noexcept
		{
			gal_assert(offset < static_cast<lang::ast_node::children_type::difference_type>(node_size(node)));
			if (auto& child = node.get_child(offset);
				child.is<lang::compiled_ast_node>()) { return *dynamic_cast<lang::compiled_ast_node&>(child).original_node; }
			else { return child; }
		}

		[[nodiscard]] inline const lang::ast_node& node_child(const lang::ast_node& node, const lang::ast_node::children_type::difference_type offset) noexcept
		{
			gal_assert(offset < static_cast<lang::ast_node::children_type::difference_type>(node_size(node)));
			if (auto& child = node.get_child(offset);
				child.is<lang::compiled_ast_node>()) { return *dynamic_cast<const lang::compiled_ast_node&>(child).original_node; }
			else { return child; }
		}

		[[nodiscard]] inline bool node_has_var_decl(const lang::ast_node& node) noexcept
		{
			if (node.is_any<lang::var_decl_ast_node, lang::assign_decl_ast_node, lang::reference_ast_node>()) { return true; }

			return std::ranges::any_of(
					node.view(),
					[](const auto& child)
					{
						// return not child.template is_any<block_ast_node, for_ast_node, ranged_for_ast_node>() &&
						//        node_has_var_decl(child);
						const auto b1 = not child.template is_any<lang::block_ast_node, lang::ranged_for_ast_node>();
						const auto b2 = node_has_var_decl(child);
						return b1 & b2;
					});
		}

		struct return_optimizer
		{
			lang::ast_node_ptr operator()(lang::ast_node_ptr p) const
			{
				if (p->is_any<lang::def_ast_node, lang::lambda_ast_node>() && not p->empty())
				{
					if (auto& back = p->back();
						back.is<lang::block_ast_node>())
					{
						if (auto& block_back = back.back();
							block_back.is<lang::return_ast_node>()) { if (block_back.size() == 1) { block_back = std::move(block_back.front()); } }
					}
				}

				return p;
			}
		};

		struct block_optimizer
		{
			lang::ast_node_ptr operator()(lang::ast_node_ptr p) const
			{
				if (p->is<lang::block_ast_node>())
				{
					if (not node_has_var_decl(*p))
					{
						if (p->size() == 1) { return std::move(p->get_child_ptr(0)); }

						return std::move(*p).remake_node<lang::no_scope_block_ast_node>();
					}
				}

				return p;
			}
		};

		struct dead_code_optimizer
		{
			lang::ast_node_ptr operator()(lang::ast_node_ptr p) const
			{
				if (p->is<lang::block_ast_node>())
				{
					auto children = p->exchange_children();

					const auto view = std::ranges::remove_if(
							children,
							[](const auto& child) { return child->template is_any<lang::noop_ast_node, lang::id_ast_node, lang::constant_ast_node>(); });
					children.erase(view.begin(), view.end());

					[[maybe_unused]] const auto result = p->exchange_children(std::move(children));
					gal_assert(result.empty());

					return std::move(*p).remake_node<lang::block_ast_node>();
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

#endif // GAL_LANG_EXTRA_OPTIMIZER_HPP
