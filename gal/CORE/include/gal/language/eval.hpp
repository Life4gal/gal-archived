#pragma once

#ifndef GAL_LANG_KITS_EVAL_HPP
#define GAL_LANG_KITS_EVAL_HPP

#include <span>
#include <memory>
#include <gal/kits/boxed_value_cast.hpp>
#include <gal/kits/dispatch.hpp>
#include <gal/language/common.hpp>
#include <utils/function.hpp>

namespace gal::lang::eval
{
	template<typename T>
	struct ast_node_impl;
	template<typename T>
	using ast_node_impl_ptr = std::unique_ptr<ast_node_impl<T>>;

	template<typename Tracer>
	struct ast_node_impl : ast_node
	{
		// although we would like to use concept in the template parameter list,
		// unfortunately we can't use it in concept without knowing the type of ast_node_impl,
		// so we can only use this unfriendly way.
		static_assert(requires(const lang::detail::dispatch_state& s, const ast_node_impl<Tracer>* n)
			{
				Tracer::trace(s, n);
			},
			"invalid tracer");

		using stack_holder = lang::detail::stack_holder;
		using dispatch_state = lang::detail::dispatch_state;

		using node_ptr_type = ast_node_impl_ptr<Tracer>;
		using children_type = std::vector<node_ptr_type>;

		friend struct ast_node_base<ast_node>;

	private:
		static const ast_node_impl& unwrap_child(const node_ptr_type& c) { return *c; }

	public:
		children_type children;

		ast_node_impl(
				const ast_node_type type,
				std::string text,
				const parse_location location,
				children_type children = {})
			: ast_node{type, std::move(text), location},
			  children{std::move(children)} {}

	protected:
		virtual kits::boxed_value do_eval(const dispatch_state& state) const { throw std::runtime_error{"un-dispatched ast_node (internal error)"}; }

	public:
		static bool get_scoped_bool_condition(
				const node_ptr_type& node,
				const dispatch_state& state
				)
		{
			stack_holder::scoped_stack_scope scoped_stack{state.stack_holder()};
			return get_bool_condition(node->eval(state), state);
		}

		[[nodiscard]] ast_node::children_type get_children() const final
		{
			ast_node::children_type ret{};
			ret.reserve(children.size());

			std::ranges::for_each(
					children,
					[&ret](const auto& child) { ret.emplace_back(*child); });

			return ret;
		}

		[[nodiscard]] kits::boxed_value eval(const dispatch_state& state) const final
		{
			try
			{
				Tracer::trace(state, this);
				return do_eval(state);
			}
			catch (eval_error& e)
			{
				e.stack_traces.push_back(*this);
				throw;
			}
		}
	};


	/**
	 * @brief Helper function that will set up the scope around a function call, including handling the named function parameters.
	 */
	namespace detail
	{
		using namespace lang::detail;


		template<typename T>
		[[nodiscard]] kits::boxed_value eval_function(
				dispatch_engine& engine,
				const ast_node_impl<T>& node,
				const kits::function_parameters& params,
				const std::span<std::string> param_names,
				const std::span<stack_holder::scope_type> locals = {},
				const bool has_this_capture = false
				)
		{
			gal_assert(params.size() == param_names.size());

			dispatch_state state{engine};

			const auto* object_this = [&]
			{
				if (auto& scope = state.stack_holder().recent_scope();
					not scope.empty() && scope.back().first == object_self_type_name::value) { return &scope.back().second; }
				if (not params.empty()) { return &params.front(); }
				return nullptr;
			}();

			stack_holder::scoped_stack_scope scoped_stack{state.stack_holder()};
			if (object_this && not has_this_capture) { state.add_object_no_check(object_self_name::value, *object_this); }

			std::ranges::for_each(
					locals,
					[&state](const auto& pair) { state.add_object_no_check(pair.first, pair.second); });

			utils::zip_invoke(
					[&state](const auto& name, const auto& object) { if (name != object_self_name::value) { state.add_object_no_check(name, object); } },
					param_names,
					params.begin());

			try { return node.eval(state); }
			catch (return_value& ret) { return std::move(ret.value); }
		}
	}
}

#endif // GAL_LANG_KITS_EVAL_HPP
