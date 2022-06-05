#pragma once

#ifndef GAL_LANG_EXTRA_OPTIMIZER_HPP
#define GAL_LANG_EXTRA_OPTIMIZER_HPP

#include <gal/foundation/ast.hpp>
#include <gal/grammar.hpp>

namespace gal::lang::addon
{
	namespace optimizer_detail
	{
		template<typename... Optimizers>
		class ast_optimizer final : public ast::ast_optimizer_base, private Optimizers...
		{
		public:
			constexpr ast_optimizer() noexcept((std::is_nothrow_default_constructible_v<Optimizers> && ...)) requires(std::is_default_constructible_v<Optimizers> && ...) = default;

			constexpr explicit ast_optimizer(Optimizers&&... optimizers)
				: Optimizers{std::forward<Optimizers>(optimizers)}... {}

			[[nodiscard]] ast::ast_node_ptr optimize(ast::ast_node_ptr node) override
			{
				((node = static_cast<Optimizers&>(*this)(std::move(node))), ...);
				return node;
			}
		};

		[[nodiscard]] inline bool node_empty(const ast::ast_node& node) noexcept
		{
			if (node.is<ast::compiled_ast_node>()) { return dynamic_cast<const ast::compiled_ast_node&>(node).original_node->empty(); }
			return node.empty();
		}

		[[nodiscard]] inline auto node_size(const ast::ast_node& node) noexcept
		{
			if (node.is<ast::compiled_ast_node>()) { return dynamic_cast<const ast::compiled_ast_node&>(node).original_node->size(); }
			return node.size();
		}

		[[nodiscard]] inline ast::ast_node& node_child(ast::ast_node& node, const ast::ast_node::children_type::difference_type offset) noexcept
		{
			gal_assert(offset < static_cast<ast::ast_node::children_type::difference_type>(node_size(node)));
			if (auto& child = node.get_child(offset);
				child.is<ast::compiled_ast_node>()) { return *dynamic_cast<ast::compiled_ast_node&>(child).original_node; }
			else { return child; }
		}

		[[nodiscard]] inline const ast::ast_node& node_child(const ast::ast_node& node, const ast::ast_node::children_type::difference_type offset) noexcept
		{
			gal_assert(offset < static_cast<ast::ast_node::children_type::difference_type>(node_size(node)));
			if (auto& child = node.get_child(offset);
				child.is<ast::compiled_ast_node>()) { return *dynamic_cast<const ast::compiled_ast_node&>(child).original_node; }
			else { return child; }
		}

		[[nodiscard]] inline bool node_has_var_decl(const ast::ast_node& node) noexcept
		{
			if (node.is_any<ast::var_decl_ast_node, ast::assign_decl_ast_node, ast::reference_ast_node>()) { return true; }

			return std::ranges::any_of(
					node.view(),
					[](const auto& child)
					{
						// return not child.template is_any<block_ast_node, for_ast_node, ranged_for_ast_node>() &&
						//        node_has_var_decl(child);
						const auto b1 = not child.template is_any<ast::block_ast_node, ast::ranged_for_ast_node>();
						const auto b2 = node_has_var_decl(child);
						return b1 & b2;
					});
		}

		struct return_optimizer
		{
			ast::ast_node_ptr operator()(ast::ast_node_ptr p) const
			{
				if (p->is_any<ast::def_ast_node, ast::lambda_ast_node>() && not p->empty())
				{
					if (auto& body = p->back();
						body.is<ast::block_ast_node>())
					{
						if (auto& ret = body.back();
							ret.is<ast::return_ast_node>())
						{
							// return with an operation
							if (ret.size() == 1)
							{
								auto children = body.exchange_children();

								// extract this operation directly into the children of the current body (the last statement will automatically return)
								auto bb = std::move(ret.get_child_ptr(grammar::return_ast_node::operation_index));
								children.pop_back();
								children.emplace_back(std::move(bb));

								[[maybe_unused]] const auto result = body.exchange_children(std::move(children));
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
			ast::ast_node_ptr operator()(ast::ast_node_ptr p) const
			{
				if (p->is<ast::block_ast_node>())
				{
					if (not node_has_var_decl(*p))
					{
						if (p->size() == 1) { return std::move(p->get_child_ptr(0)); }

						return std::move(*p).remake_node<ast::no_scope_block_ast_node>();
					}
				}

				return p;
			}
		};

		struct dead_code_optimizer
		{
			ast::ast_node_ptr operator()(ast::ast_node_ptr p) const
			{
				if (p->is<ast::block_ast_node>())
				{
					auto children = p->exchange_children();

					const auto view = std::ranges::remove_if(
							children,
							[](const auto& child) { return child->template is_any<ast::noop_ast_node, ast::id_ast_node, ast::constant_ast_node>(); });
					children.erase(view.begin(), view.end());

					[[maybe_unused]] const auto result = p->exchange_children(std::move(children));
					gal_assert(result.empty());

					return std::move(*p).remake_node<ast::block_ast_node>();
				}

				return p;
			}
		};

		struct unused_return_optimizer
		{
			ast::ast_node_ptr operator()(ast::ast_node_ptr p) const
			{
				if (
					p->is_any<ast::block_ast_node, ast::no_scope_block_ast_node>() &&
					not p->empty())
				{
					auto children = p->exchange_children();

					for (auto& i: children) { if (i->is<ast::fun_call_ast_node>()) { i = std::move(*i).remake_node<ast::unused_return_fun_call_ast_node>(); } }

					[[maybe_unused]] const auto result = p->exchange_children(std::move(children));
					gal_assert(result.empty());
				}
				else if (
					p->is_any<ast::ranged_for_ast_node, ast::while_ast_node>())
				{
					if (const auto size = node_size(*p); size > 0)
					{
						if (auto& child = node_child(*p, static_cast<ast::ast_node::children_type::difference_type>(size) - 1);
							child.is_any<ast::block_ast_node, ast::no_scope_block_ast_node>())
						{
							auto children = child.exchange_children();

							const auto child_size = node_size(child);
							for (decltype(node_size(child)) i = 0; i < child_size; ++i)
							{
								if (auto& sub_child = node_child(child, static_cast<ast::ast_node::children_type::difference_type>(i));
									sub_child.is<ast::fun_call_ast_node>()) { children[i] = std::move(*children[i]).remake_node<ast::unused_return_fun_call_ast_node>(); }
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
			ast::ast_node_ptr operator()(ast::ast_node_ptr p) const
			{
				if (
					p->is<ast::equation_ast_node>() &&
					p->identifier() == foundation::operator_assign_name::value &&
					p->size() == 2 &&
					p->get_child(grammar::equation_ast_node::lhs_index).is<ast::var_decl_ast_node>())
				{
					auto children = p->exchange_children();

					children[grammar::equation_ast_node::lhs_index] = std::move(children[grammar::equation_ast_node::lhs_index]->get_child_ptr(grammar::var_decl_ast_node::index));

					[[maybe_unused]] const auto result = p->exchange_children(std::move(children));
					gal_assert(result.empty());

					return std::move(*p).remake_node<ast::assign_decl_ast_node>();
				}

				return p;
			}
		};

		struct constant_if_optimizer
		{
			ast::ast_node_ptr operator()(ast::ast_node_ptr p) const
			{
				if (
					p->is<ast::if_ast_node>() &&
					p->size() >= 2 &&
					p->get_child(grammar::if_ast_node::condition_index).is<ast::constant_ast_node>())
				{
					if (const auto& condition = dynamic_cast<ast::constant_ast_node&>(p->get_child(grammar::if_ast_node::condition_index)).value;
						// todo: maybe not just boolean?
						condition.type_info().bare_equal(typeid(bool)))
					{
						if (boxed_cast<bool>(condition)) { return std::move(p->get_child_ptr(grammar::if_ast_node::true_branch_index)); }
						// condition evaluates to false
						if (p->size() == 3) { return std::move(p->get_child_ptr(grammar::if_ast_node::false_branch_index)); }
					}
				}

				return p;
			}
		};

		struct binary_fold_optimizer
		{
			ast::ast_node_ptr operator()(ast::ast_node_ptr p) const
			{
				if (
					p->is<ast::binary_operator_ast_node>() &&
					p->size() == 2 &&
					not p->get_child(grammar::binary_operator_ast_node::lhs_index).is<ast::constant_ast_node>() &&
					p->get_child(grammar::binary_operator_ast_node::rhs_index).is<ast::constant_ast_node>()
				)
				{
					if (const auto parsed = foundation::algebraic_operation(p->identifier());
						parsed != foundation::algebraic_operations::unknown)
					{
						if (const auto& rhs = dynamic_cast<ast::constant_ast_node&>(p->get_child(grammar::binary_operator_ast_node::rhs_index)).value;
							rhs.type_info().is_arithmetic()) { return std::move(*p).remake_node<ast::fold_right_binary_operator_ast_node>(rhs); }
					}
				}

				return p;
			}
		};

		struct constant_fold_optimizer
		{
			ast::ast_node_ptr operator()(ast::ast_node_ptr p) const
			{
				// todo: this will generates a new identifier, but how do we store it?

				if (
					p->is<ast::unary_operator_ast_node>() &&
					p->size() == 1 &&
					p->get_child(grammar::unary_operator_ast_node::index).is<ast::constant_ast_node>()
				)
				{
					const auto parsed = foundation::algebraic_operation(p->identifier(), true);
					const auto& lhs = dynamic_cast<const ast::constant_ast_node&>(p->get_child(grammar::unary_operator_ast_node::index)).value;
					// const auto	match  = ast::ast_node::text_type{p->identifier()}.append(p->front().identifier());

					if (
						parsed != foundation::algebraic_operations::unknown &&
						parsed != foundation::algebraic_operations::bitwise_and &&
						lhs.type_info().is_arithmetic()) { return std::move(*p).remake_node<ast::constant_ast_node>(types::number_type::unary_invoke(lhs, parsed)); }

					if (
						lhs.type_info().bare_equal(typeid(bool)) &&
						p->identifier() == foundation::operator_unary_not_name::value) { return std::move(*p).remake_node<ast::constant_ast_node>(foundation::boxed_value{not boxed_cast<bool>(lhs)}); }
				}
				else if (
					p->is_any<ast::logical_and_ast_node, ast::logical_or_ast_node>() &&
					p->size() == 2 &&
					p->get_child(grammar::logical_and_ast_node::lhs_index).is<ast::constant_ast_node>() &&
					p->get_child(grammar::logical_and_ast_node::rhs_index).is<ast::constant_ast_node>()
				)
				{
					const auto& lhs = dynamic_cast<const ast::constant_ast_node&>(p->get_child(grammar::logical_and_ast_node::lhs_index)).value;
					const auto& rhs = dynamic_cast<const ast::constant_ast_node&>(p->get_child(grammar::logical_and_ast_node::rhs_index)).value;
					// todo: maybe not just boolean?
					if (lhs.type_info().bare_equal(typeid(bool)) && rhs.type_info().bare_equal(typeid(bool)))
					{
						// const auto match = ast::ast_node::text_type{p->front().identifier()}.append(" ").append(p->identifier()).append(" ").append(p->back().identifier());

						return std::move(*p).remake_node<ast::constant_ast_node>(
								[l = boxed_cast<bool>(lhs), r = boxed_cast<bool>(rhs)](const ast::ast_node& n)
								{
									if (n.is<ast::logical_and_ast_node>()) { return foundation::boxed_value{l && r}; }
									return foundation::boxed_value{l || r};
								}(*p));
					}
				}
				else if (
					p->is<ast::binary_operator_ast_node>() &&
					p->size() == 2 &&
					p->get_child(grammar::binary_operator_ast_node::lhs_index).is<ast::constant_ast_node>() &&
					p->get_child(grammar::binary_operator_ast_node::rhs_index).is<ast::constant_ast_node>()
				)
				{
					const auto parsed = foundation::algebraic_operation(p->identifier());
					if (parsed != foundation::algebraic_operations::unknown)
					{
						const auto& lhs = dynamic_cast<const ast::constant_ast_node&>(p->get_child(grammar::binary_operator_ast_node::lhs_index)).value;
						const auto& rhs = dynamic_cast<const ast::constant_ast_node&>(p->get_child(grammar::binary_operator_ast_node::rhs_index)).value;
						if (lhs.type_info().is_arithmetic() && rhs.type_info().is_arithmetic())
						{
							// const auto match = ast::ast_node::text_type{p->front().identifier()}.append(" ").append(p->identifier()).append(" ").append(p->back().identifier());

							return std::move(*p).remake_node<ast::constant_ast_node>(types::number_type::binary_invoke(parsed, lhs, rhs));
						}
					}
				}
				// conversion of numeric literals
				else if (
					p->is<ast::fun_call_ast_node>() &&
					p->size() == 2 &&
					// p->get_child(grammar::fun_call_ast_node::function_name_index).is<ast::id_ast_node>() &&
					// p->get_child(grammar::fun_call_ast_node::arg_list_index).is<ast::arg_list_ast_node>() &&
					p->get_child(grammar::fun_call_ast_node::arg_list_index).size() == 1 &&
					p->get_child(grammar::fun_call_ast_node::arg_list_index).get_child(0).is<ast::constant_ast_node>()
				)
				{
					if (const auto& arg = dynamic_cast<const ast::constant_ast_node&>(p->get_child(grammar::fun_call_ast_node::arg_list_index).get_child(0)).value;
						arg.type_info().is_arithmetic())
					{
						// const auto match = ast::ast_node::text_type{p->front().identifier()}.append("(").append(p->back().front().identifier()).append(")");

						if (const auto& name = p->get_child(grammar::fun_call_ast_node::function_index).identifier();
							name == "double") { return std::move(*p).remake_node<ast::constant_ast_node>(const_var(types::number_type{arg}.as<double>())); }
						else if (name == "int") { return std::move(*p).remake_node<ast::constant_ast_node>(const_var(types::number_type{arg}.as<int>())); }
						// todo: more type
					}
				}

				return p;
			}
		};

		// todo: more optimizer
	}// namespace optimizer_detail

	using ast_optimizer = optimizer_detail::ast_optimizer<
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
