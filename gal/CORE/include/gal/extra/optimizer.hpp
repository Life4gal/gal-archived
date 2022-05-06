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
						if (back.back().is<lang::return_ast_node>())
						{
							if (back.back().size() == 1)
							{
								auto children = back.exchange_children();

								auto bb = std::move(back.back().front_ptr());
								children.pop_back();
								children.emplace_back(std::move(bb));

								[[maybe_unused]] const auto result = back.exchange_children(std::move(children));
								gal_assert(result.empty());
							}
						}
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

		struct unused_return_optimizer
		{
			lang::ast_node_ptr operator()(lang::ast_node_ptr p) const
			{
				if (
					p->is_any<lang::block_ast_node, lang::no_scope_block_ast_node>() &&
					not p->empty())
				{
					auto children = p->exchange_children();

					for (decltype(children.size()) i = 0; i < children.size(); ++i) { if (children[i]->is<lang::fun_call_ast_node>()) { children[i] = std::move(*children[i]).remake_node<lang::unused_return_fun_call_ast_node>(); } }

					[[maybe_unused]] const auto result = p->exchange_children(std::move(children));
					gal_assert(result.empty());
				}
				else if (
					p->is_any<lang::ranged_for_ast_node, lang::while_ast_node>())
				{
					if (const auto size = node_size(*p); size > 0)
					{
						if (auto& child = node_child(*p, static_cast<lang::ast_node::children_type::difference_type>(size) - 1);
							child.is_any<lang::block_ast_node, lang::no_scope_block_ast_node>())
						{
							auto children = child.exchange_children();

							const auto child_size = node_size(child);
							for (decltype(node_size(child)) i = 0; i < child_size; ++i)
							{
								if (auto& sub_child = node_child(child, static_cast<lang::ast_node::children_type::difference_type>(i));
									sub_child.is<lang::fun_call_ast_node>()) { children[i] = std::move(*children[i]).remake_node<lang::unused_return_fun_call_ast_node>(); }
							}

							[[maybe_unused]] const auto result = child.exchange_children(std::move(children));
							gal_assert(result.empty());
						}
					}
				}

				return p;
			}
		};

		struct assign_decl_optimizer
		{
			lang::ast_node_ptr operator()(lang::ast_node_ptr p) const
			{
				if (
					p->is<lang::equation_ast_node>() &&
					p->identifier() == lang::operator_assign_name::value &&
					p->size() == 2 &&
					p->front().is<lang::var_decl_ast_node>())
				{
					auto children = p->exchange_children();

					children[0] = std::move(children[0]->front_ptr());

					[[maybe_unused]] const auto result = p->exchange_children(std::move(children));
					gal_assert(result.empty());

					return std::move(*p).remake_node<lang::assign_decl_ast_node>();
				}

				return p;
			}
		};

		struct constant_if_optimizer
		{
			lang::ast_node_ptr operator()(lang::ast_node_ptr p) const
			{
				if (
					p->is<lang::if_ast_node>() &&
					p->size() >= 2 &&
					p->front().is<lang::constant_ast_node>())
				{
					if (const auto& condition = dynamic_cast<lang::constant_ast_node&>(p->front()).value;
						condition.type_info().bare_equal(typeid(bool)))
					{
						if (boxed_cast<bool>(condition)) { return std::move(p->get_child_ptr(1)); }
						if (p->size() == 3) { return std::move(p->get_child_ptr(2)); }
					}
				}

				return p;
			}
		};

		struct binary_fold_optimizer
		{
			lang::ast_node_ptr operator()(lang::ast_node_ptr p) const
			{
				if (
					p->is<lang::binary_operator_ast_node>() &&
					p->size() == 2 &&
					not p->front().is<lang::constant_ast_node>() &&
					p->back().is<lang::constant_ast_node>()
				)
				{
					if (const auto parsed = lang::algebraic_operation(p->identifier());
						parsed != lang::algebraic_operations::unknown)
					{
						if (const auto& rhs = dynamic_cast<lang::constant_ast_node&>(p->back()).value;
							rhs.type_info().is_arithmetic()) { return std::move(*p).remake_node<lang::fold_right_binary_operator_ast_node>(rhs); }
					}
				}

				return p;
			}
		};

		struct constant_fold_optimizer
		{
			lang::ast_node_ptr operator()(lang::ast_node_ptr p) const
			{
				// todo: this will generates a new identifier, but how do we store it?

				if (
					p->is<lang::unary_operator_ast_node>() &&
					p->size() == 1 &&
					p->front().is<lang::constant_ast_node>()
				)
				{
					const auto parsed = lang::algebraic_operation(p->identifier(), true);
					const auto& lhs = dynamic_cast<const lang::constant_ast_node&>(p->front()).value;
					// const auto	match  = lang::ast_node::text_type{p->identifier()}.append(p->front().identifier());

					if (
						parsed != lang::algebraic_operations::unknown &&
						parsed != lang::algebraic_operations::bitwise_and &&
						lhs.type_info().is_arithmetic()) { return std::move(*p).remake_node<lang::constant_ast_node>(foundation::boxed_number::unary_invoke(lhs, parsed)); }

					if (
						lhs.type_info().bare_equal(typeid(bool)) &&
						p->identifier() == lang::operator_unary_not_name::value) { return std::move(*p).remake_node<lang::constant_ast_node>(foundation::boxed_value{not boxed_cast<bool>(lhs)}); }
				}
				else if (
					p->is_any<lang::logical_and_ast_node, lang::logical_or_ast_node>() &&
					p->size() == 2 &&
					p->front().is<lang::constant_ast_node>() &&
					p->back().is<lang::constant_ast_node>()
				)
				{
					const auto& lhs = dynamic_cast<const lang::constant_ast_node&>(p->front()).value;
					const auto& rhs = dynamic_cast<const lang::constant_ast_node&>(p->back()).value;
					if (lhs.type_info().bare_equal(typeid(bool)) && rhs.type_info().bare_equal(typeid(bool)))
					{
						// const auto match = lang::ast_node::text_type{p->front().identifier()}.append(" ").append(p->identifier()).append(" ").append(p->back().identifier());

						return std::move(*p).remake_node<lang::constant_ast_node>(
								[l = boxed_cast<bool>(lhs), r = boxed_cast<bool>(rhs)](const lang::ast_node& n)
								{
									if (n.is<lang::logical_and_ast_node>()) { return foundation::boxed_value{l && r}; }
									return foundation::boxed_value{l || r};
								}(*p));
					}
				}
				else if (
					p->is<lang::binary_operator_ast_node>() &&
					p->size() == 2 &&
					p->front().is<lang::constant_ast_node>() &&
					p->back().is<lang::constant_ast_node>()
				)
				{
					const auto parsed = lang::algebraic_operation(p->identifier());
					if (parsed != lang::algebraic_operations::unknown)
					{
						const auto& lhs = dynamic_cast<const lang::constant_ast_node&>(p->front()).value;
						const auto& rhs = dynamic_cast<const lang::constant_ast_node&>(p->back()).value;
						if (lhs.type_info().is_arithmetic() && rhs.type_info().is_arithmetic())
						{
							// const auto match = lang::ast_node::text_type{p->front().identifier()}.append(" ").append(p->identifier()).append(" ").append(p->back().identifier());

							return std::move(*p).remake_node<lang::constant_ast_node>(foundation::boxed_number::binary_invoke(parsed, lhs, rhs));
						}
					}
				}
				else if (
					p->is<lang::fun_call_ast_node>() &&
					p->size() == 2 &&
					p->front().is<lang::id_ast_node>() &&
					p->back().is<lang::arg_list_ast_node>() &&
					p->back().size() == 1 &&
					p->back().front().is<lang::constant_ast_node>()
				)
				{
					if (const auto& arg = dynamic_cast<const lang::constant_ast_node&>(p->back().front()).value;
						arg.type_info().is_arithmetic())
					{
						// const auto match = lang::ast_node::text_type{p->front().identifier()}.append("(").append(p->back().front().identifier()).append(")");

						if (const auto& name = p->front().identifier();
							name == "double") { return std::move(*p).remake_node<lang::constant_ast_node>(const_var(foundation::boxed_number{arg}.as<double>())); }
						else if (name == "int") { return std::move(*p).remake_node<lang::constant_ast_node>(const_var(foundation::boxed_number{arg}.as<int>())); }
						// todo: more type
					}
				}

				return p;
			}
		};

		// todo: more optimizer
	}// namespace optimizer_detail

	using default_optimizer = optimizer_detail::default_optimizer<
		optimizer_detail::return_optimizer,
		optimizer_detail::block_optimizer,
		optimizer_detail::dead_code_optimizer,
		optimizer_detail::unused_return_optimizer,
		optimizer_detail::assign_decl_optimizer,
		optimizer_detail::constant_if_optimizer,
		optimizer_detail::binary_fold_optimizer,
		optimizer_detail::constant_fold_optimizer
	>;
};

#endif // GAL_LANG_EXTRA_OPTIMIZER_HPP
