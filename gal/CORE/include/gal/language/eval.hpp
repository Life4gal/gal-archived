#pragma once

#ifndef GAL_LANG_KITS_EVAL_HPP
#define GAL_LANG_KITS_EVAL_HPP

#include <span>
#include <memory>
#include <gal/kits/boxed_value_cast.hpp>
#include <gal/kits/dispatch.hpp>
#include <gal/utility/type_info.hpp>
#include <gal/language/common.hpp>
#include <gal/kits/dynamic_object_function.hpp>
#include <utils/function.hpp>

namespace gal::lang::eval
{
	template<typename T>
	struct ast_node_impl_base;
	template<typename T>
	using ast_node_impl_ptr = std::unique_ptr<ast_node_impl_base<T>>;

	template<typename Tracer>
	struct ast_node_impl_base : ast_node
	{
		// although we would like to use concept in the template parameter list,
		// unfortunately we can't use it in concept without knowing the type of ast_node_impl,
		// so we can only use this unfriendly way.
		static_assert(requires(const lang::detail::dispatch_state& s, const ast_node_impl_base<Tracer>* n)
			{
				Tracer::trace(s, n);
			},
			"invalid tracer");

		using stack_holder = lang::detail::stack_holder;
		using dispatch_engine = lang::detail::dispatch_engine;
		using dispatch_state = lang::detail::dispatch_state;

		using node_ptr_type = ast_node_impl_ptr<Tracer>;
		using children_type = std::vector<node_ptr_type>;

		friend struct ast_node_base<ast_node>;

	protected:
		static const ast_node_impl_base& unwrap_child(const node_ptr_type& c) { return *c; }

	public:
		children_type children;

		ast_node_impl_base(
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
				const ast_node_impl_base<T>& node,
				const kits::function_parameters& params,
				const std::span<std::string_view> param_names,
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
					[&state](const auto& pair) { (void)state.add_object_no_check(pair.first, pair.second); });

			utils::zip_invoke(
					[&state](const auto& name, const auto& object) { if (name != object_self_name::value) { state.add_object_no_check(name, object); } },
					param_names,
					params.begin());

			try { return node.eval(state); }
			catch (return_value& ret) { return std::move(ret.value); }
		}

		[[nodiscard]] inline kits::boxed_value clone_if_necessary(
				kits::boxed_value incoming,
				dispatch_engine::location_type& location,
				const dispatch_state& state
				)
		{
			if (not incoming.is_return_value())
			{
				if (const auto& ti = incoming.type_info(); ti.is_arithmetic()) { return kits::boxed_number::clone(incoming); }
				else if (ti.bare_equal(typeid(bool))) { return kits::boxed_value{*static_cast<const bool*>(incoming.get_const_ptr())}; }
				else if (ti.bare_equal(typeid(std::string))) { return kits::boxed_value{*static_cast<const std::string*>(incoming.get_const_ptr())}; }
				return state->call_function(object_clone_interface_name::value, location, kits::function_parameters{incoming}, state.conversion());
			}
			incoming.reset_return_value();
			return incoming;
		}
	}

	template<typename T>
	struct id_ast_node final : ast_node_impl_base<T>
	{
	private:
		mutable typename ast_node_impl_base<T>::dispatch_engine::location_type location_{};

	public:
		id_ast_node(const std::string_view text, parse_location location)
			: ast_node_impl_base{ast_node_type::id_t, text, std::move(location)} {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override
		{
			try { return state.get_object(this->text, location_); }
			catch (std::exception&) { throw eval_error{std_format::format("Can not find object '{}'", this->text)}; }
		}
	};

	template<typename T>
	struct constant_ast_node final : ast_node_impl_base<T>
	{
		kits::boxed_value value;

		constant_ast_node(
				const std::string_view text,
				parse_location location,
				kits::boxed_value value)
			: ast_node_impl_base{ast_node_type::constant_t, text, std::move(location)},
			  value{std::move(value)} {}

		explicit constant_ast_node(kits::boxed_value value)
			: constant_ast_node({}, {}, std::move(value)) {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override { return value; }
	};

	template<typename T>
	struct compiled_ast_node final : ast_node_impl_base<T>
	{
		using original_node_type = ast_node_impl_ptr<T>;
		using function_type = std::function<kits::boxed_value(const typename ast_node_impl_base<T>::children_type&, const typename ast_node_impl_base<T>::dispatch_state&)>;

		original_node_type original_node;
		function_type function;

		compiled_ast_node(
				original_node_type original_node,
				typename ast_node_impl_base<T>::children_type children,
				function_type function
				)
			: ast_node_impl_base{
					  ast_node_type::compiled_t,
					  original_node->text,
					  original_node->location,
					  std::move(children)
			  },
			  original_node{std::move(original_node)},
			  function{std::move(function)} {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override { return function(this->children, state); }
	};

	template<typename T>
	struct fold_right_binary_operator_ast_node final : ast_node_impl_base<T>
	{
	private:
		algebraic_invoker::operations operation_;
		kits::boxed_value rhs_;

		mutable typename ast_node_impl_base<T>::dispatch_engine::location_type location_{};

		kits::boxed_value do_operation(
				typename ast_node_impl_base<T>::dispatch_state& state,
				const std::string_view operation,
				const kits::boxed_value& lhs
				) const
		{
			try
			{
				if (lhs.type_info().is_arithmetic())
				{
					// If it's an arithmetic operation we want to short circuit dispatch
					try
					{
						return kits::boxed_number::binary_invoke(
								operation_,
								lhs,
								rhs_);
					}
					catch (const kits::arithmetic_error&) { throw; }catch (...) { throw eval_error{std_format::format("Error with numeric operator '{}' called", operation)}; }
				}

				typename ast_node_impl_base<T>::stack_holder::scoped_function_scope function_scope{state};

				const auto params = kits::function_parameters{lhs, rhs_};

				function_scope.push_params(params);
				return state->call_function(operation, location_, params, state.conversion());
			}
			catch (const kits::dispatch_error& e) { throw eval_error{std_format::format("Can not find appropriate '{}' operator", operation_), e.parameters, e.functions, false, *state}; }
		}

	public:
		fold_right_binary_operator_ast_node(
				const algebraic_invoker::operation_string_type operation,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children,
				kits::boxed_value rhs)
			: ast_node_impl_base{ast_node_type::binary_t, operation, std::move(location), std::move(children)},
			  operation_{algebraic_invoker::to_operation(operation)},
			  rhs_{std::move(rhs)} {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override { return do_operation(state, this->text, ast_node_impl_base<T>::unwrap_child(this->children.front()).eval(state)); }
	};

	template<typename T>
	struct binary_operator_ast_node final : ast_node_impl_base<T>
	{
	private:
		algebraic_invoker::operations operation_;

		mutable typename ast_node_impl_base<T>::dispatch_engine::location_type location_{};

		kits::boxed_value do_operation(
				typename ast_node_impl_base<T>::dispatch_state& state,
				const algebraic_invoker::operations operation,
				const std::string_view operation_string,
				const kits::boxed_value& lhs,
				const kits::boxed_value& rhs
				) const
		{
			try
			{
				if (operation != algebraic_invoker::operations::unknown && lhs.type_info().is_arithmetic() && rhs.type_info().is_arithmetic())
				{
					// If it's an arithmetic operation we want to short circuit dispatch
					try { return kits::boxed_number::binary_invoke(operation, lhs, rhs); }
					catch (const kits::arithmetic_error&) { throw; }
					catch (...) { throw eval_error{std_format::format("Error with numeric operator '{}' called", operation_string)}; }
				}

				typename ast_node_impl_base<T>::stack_holder::scoped_function_scope function_scope{state};

				const auto params = kits::function_parameters{lhs, rhs};

				function_scope.push_params(params);
				return state->call_function(operation_string, location_, params, state.conversion());
			}
			catch (const kits::dispatch_error& e) { throw eval_error{std_format::format("Can not find appropriate '{}' operator", operation_string), e.parameters, e.functions, false, *state}; }
		}

	public:
		binary_operator_ast_node(
				const algebraic_invoker::operation_string_type operation,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{operation, ast_node_type::binary_t, std::move(location), std::move(children)},
			  operation_{algebraic_invoker::to_operation(operation)} {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override { return do_operation(state, operation_, this->text, ast_node_impl_base<T>::unwrap_child(this->children[0]).eval(state), ast_node_impl_base<T>::unwrap_child(this->children[1]).eval(state)); }
	};

	template<typename T>
	struct fun_call_ast_node : ast_node_impl_base<T>
	{
	protected:
		template<bool SaveParams>
		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const
		{
			typename ast_node_impl_base<T>::stack_holder::scoped_function_scope function_scope{state};

			std::vector<kits::boxed_value> params;
			params.reserve(ast_node_impl_base<T>::unwrap_child(this->children[1]).children.size());
			std::ranges::for_each(
					ast_node_impl_base<T>::unwrap_child(this->children[1]).children,
					[&params, &state](const auto& child) { params.push_back(child.eval(state)); });

			if constexpr (SaveParams) { function_scope.push_params(kits::function_parameters{params}); }
			else { }

			kits::boxed_value function{ast_node_impl_base<T>::unwrap_child(this->children.front()).eval(state)};

			try { return (*state->template boxed_cast<const kits::proxy_function_base*>(function))(kits::function_parameters{params}, state.conversion()); }
			catch (const kits::dispatch_error& e) { throw eval_error{std_format::format("{} with function '{}' called", e.what(), ast_node_impl_base<T>::unwrap_child(this->children.front()).text), e.parameters, e.functions, false, *state}; }
			catch (const kits::bad_boxed_cast&)
			{
				try
				{
					// handle the case where there is only 1 function to try to call and dispatch fails on it
					throw eval_error{
							std_format::format("Error with function '{}' called", ast_node_impl_base<T>::unwrap_child(this->children.front()).text),
							params,
							kits::proxy_function_base::contained_functions_type{state->template boxed_cast<const kits::proxy_function_base::contained_functions_type::value_type&>(function)},
							false,
							*state};
				}
				catch (const kits::bad_boxed_cast&)
				{
					throw eval_error{
							std_format::format("'{}' does not evaluate to a function", ast_node_impl_base<T>::unwrap_child(this->children.front()).pretty_print())};
				}
			}
			catch (const kits::arity_error& e)
			{
				throw eval_error{
						std_format::format("{} with function '{}' called", e.what(), ast_node_impl_base<T>::unwrap_child(this->children.front()).text)
				};
			}
			catch (const kits::guard_error& e)
			{
				throw eval_error{
						std_format::format("{} with function '{}' called", e.what(), ast_node_impl_base<T>::unwrap_child(this->children.front()).text)};
			}
			catch (detail::return_value& ret) { return std::move(ret.value); }
		}

	public:
		fun_call_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{ast_node_type::fun_call_t, text, std::move(location), std::move(children)} { gal_assert(not this->children.empty()); }

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override { return do_eval<true>(state); }
	};

	template<typename T>
	struct unused_return_fun_call_ast_node final : fun_call_ast_node<T>
	{
		using fun_call_ast_node<T>::fun_call_ast_node;

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override { return this->template do_eval<false>(state); }
	};

	template<typename T>
	struct arg_ast_node final : ast_node_impl_base<T>
	{
		arg_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{ast_node_type::arg_t, text, std::move(location), std::move(children)} {}
	};

	template<typename T>
	struct arg_list_ast_node final : ast_node_impl_base<T>
	{
		arg_list_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{ast_node_type::arg_list_t, text, std::move(location), std::move(children)} {}

		static std::string_view get_arg_name(const ast_node_impl_base<T>& node)
		{
			if (node.children.empty()) { return node.text; }
			if (node.children.size() == 1) { return node.children.front()->text; }
			return node.children[1]->text;
		}

		static std::vector<std::string_view> get_arg_names(const ast_node_impl_base<T>& node)
		{
			std::vector<decltype(get_arg_name(node))> ret;
			ret.reserve(node.children.size());

			std::ranges::for_each(
					node.children,
					[&ret](const auto& child) { ret.push_back(get_arg_name(ast_node_impl_base<T>::unwrap_child(child))); });

			return ret;
		}

		static std::pair<std::string_view, utility::gal_type_info> get_arg_type(const ast_node_impl_base<T>& node, const typename ast_node_impl_base<T>::dispatch_state& state)
		{
			if (node.children.size() < 2) { return {}; }
			return {node.children.front()->text, state->get_type_info(node.children.front()->text, false)};
		}

		static kits::param_types get_arg_types(const ast_node_impl_base<T>& node, const typename ast_node_impl_base<T>::dispatch_state& state)
		{
			std::vector<decltype(get_arg_type(node, state))> ret;
			ret.reserve(node.children.size());

			std::ranges::for_each(
					node.children,
					[&ret, &state](const auto& child) { ret.push_back(get_arg_type(ast_node_impl_base<T>::unwrap_child(child), state)); });

			return kits::param_types{ret};
		}
	};

	template<typename T>
	struct equation_ast_node final : ast_node_impl_base<T>
	{
	private:
		algebraic_invoker::operations operation_;

		mutable typename ast_node_impl_base<T>::dispatch_engine::location_type location_{};
		mutable typename ast_node_impl_base<T>::dispatch_engine::location_type clone_location_{};

	public:
		equation_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{ast_node_type::equation_t, text, std::move(location), std::move(children)},
			  operation_{algebraic_invoker::to_operation(this->text)} { gal_assert(this->children.size() == 2); }

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override
		{
			typename ast_node_impl_base<T>::stack_holder::scoped_function_scope function_scope{state};

			auto params = [&]
			{
				// The RHS *must* be evaluated before the LHS
				// consider `var range = range(x)`
				// if we declare the variable in scope first, then the name lookup fails
				// for the RHS
				auto rhs = ast_node_impl_base<T>::unwrap_child(this->children[1]).eval(state);
				auto lhs = ast_node_impl_base<T>::unwrap_child(this->children[0]).eval(state);
				return kits::function_parameters{lhs, rhs};
			}();

			if (params[0].is_return_value()) { throw eval_error{"Error, can not assign to a temporary value"}; }
			if (params[0].is_const()) { throw eval_error{"Error, can not assign to a immutable value"}; }

			if (operation_ != algebraic_invoker::operations::unknown && params[0].type_info().is_arithmetic() && params[1].type_info().is_arithmetic())
			{
				try { return kits::boxed_number::binary_invoke(operation_, params[0], params[1]); }
				catch (const std::exception&) { throw eval_error{"Error with unsupported arithmetic assignment operation"}; }
			}
			if (operation_ == algebraic_invoker::operations::assign)
			{
				try
				{
					if (params[0].is_undefined())
					{
						if (not this->children.empty() &&
						    (ast_node_impl_base<T>::unwrap_child(this->children.front()).type == ast_node_type::reference_t ||
						     (not ast_node_impl_base<T>::unwrap_child(this->children.front()).children.empty() && ast_node_impl_base<T>::unwrap_child(ast_node_impl_base<T>::unwrap_child(this->children.front()).children.front()).type == ast_node_type::reference_t))
						)
						{
							// todo: This does not handle the case of an unassigned reference variable being assigned outside of its declaration
							params[0].assign(params[1]).reset_return_value();
							return params[0];
						}
						params[1] = detail::clone_if_necessary(std::move(params[1]), clone_location_, state);
					}

					try { return state->call_function(this->text, location_, params, state.conversion()); }
					catch (const kits::dispatch_error& e)
					{
						throw eval_error{
								std_format::format("Can not find appropriate '{}' operator", this->text),
								e.parameters,
								e.functions,
								false,
								*state};
					}
				}
				catch (const kits::dispatch_error& e)
				{
					throw eval_error{
							"Missing clone or copy constructor for right hand side of equation",
							e.parameters,
							e.functions,
							false,
							*state};
				}
			}
			if (this->text == operator_assign_if_type_match_name::value)
			{
				if (params[0].is_undefined() || kits::boxed_value::is_type_match(params[0], params[1]))
				{
					params[0].assign(params[1]).reset_return_value();
					return params[0];
				}
				throw eval_error{"Mismatched types in equation"};
			}

			try { return state->call_function(this->text, location_, params, state.conversion()); }
			catch (const kits::dispatch_error& e)
			{
				throw eval_error{
						std_format::format("Can not find appropriate '{}' operator", this->text),
						e.parameters,
						e.functions,
						false,
						*state};
			}
		}
	};

	template<typename T>
	struct global_decl_ast_node final : ast_node_impl_base<T>
	{
		global_decl_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{ast_node_type::global_decl_t, text, std::move(location), std::move(children)} {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override
		{
			const auto name = [&]() -> std::string_view
			{
				if (ast_node_impl_base<T>::unwrap_child(this->children.front()).type == ast_node_type::reference_t) { return ast_node_impl_base<T>::unwrap_child(ast_node_impl_base<T>::unwrap_child(this->children.front()).children.front()).text; }
				return ast_node_impl_base<T>::unwrap_child(this->children.front()).text;
			}();

			return state->add_global_mutable_no_throw(name, {});
		}
	};

	template<typename T>
	struct var_decl_ast_node final : ast_node_impl_base<T>
	{
		var_decl_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{ast_node_type::var_decl_t, text, std::move(location), std::move(children)} {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override
		{
			const auto& name = ast_node_impl_base<T>::unwrap_child(this->children.front()).text;

			try
			{
				kits::boxed_value object;
				state.add_object_no_check(name, object);
				return object;
			}
			catch (const name_conflict_error& e) { throw eval_error{std_format::format("Variable redefined '{}'", e.which())}; }
		}
	};

	template<typename T>
	struct assign_decl_ast_node final : ast_node_impl_base<T>
	{
	private:
		mutable typename ast_node_impl_base<T>::dispatch_engine::location_type location_{};

	public:
		assign_decl_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{ast_node_type::assign_decl_t, text, std::move(location), std::move(children)} {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override
		{
			const auto& name = ast_node_impl_base<T>::unwrap_child(this->children.front()).text;

			try
			{
				auto object = detail::clone_if_necessary(ast_node_impl_base<T>::unwrap_child(this->children[1]).eval(state), location_, state);
				object.reset_return_value();
				state.add_object_no_check(name, object);
				return object;
			}
			catch (const name_conflict_error& e) { throw eval_error{std_format::format("Variable redefined '{}'", e.which())}; }
		}
	};

	template<typename T>
	struct class_decl_ast_node final : ast_node_impl_base<T>
	{
		class_decl_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{ast_node_type::class_decl_t, text, std::move(location), std::move(children)} {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override
		{
			typename ast_node_impl_base<T>::stack_holder::scoped_scope scoped_scope{state};

			// todo: do this better
			// put class name in current scope, so it can be looked up by the attrs and methods
			state.add_object_no_check("_current_class_name", kits::const_var(ast_node_impl_base<T>::unwrap_child(this->children.front()).text));

			ast_node_impl_base<T>::unwrap_child(this->children[1]).eval(state);

			return kits::void_var();
		}
	};

	template<typename T>
	struct def_ast_node final : ast_node_impl_base<T>
	{
		using shared_node_type = std::shared_ptr<ast_node_impl_base<T>>;

		shared_node_type body_node;
		shared_node_type guard_node;

		[[nodiscard]] static shared_node_type get_body_node(typename ast_node_impl_base<T>::children_type&& children) { return std::move(children.back()); }

		[[nodiscard]] static bool has_guard_node(const typename ast_node_impl_base<T>::children_type& children, const typename ast_node_impl_base<T>::children_type::size_type offset) noexcept
		{
			if (children.size() > offset + 2)
			{
				if (children[offset + 1]->type != ast_node_type::arg_list_t) { return true; }
				if (children.size() > offset + 3) { return true; }
			}
			return false;
		}

		[[nodiscard]] static shared_node_type get_guard_node(typename ast_node_impl_base<T>::children_type&& children, const bool has_guard)
		{
			if (has_guard) { return std::move(*(children.end() - 2)); }
			return {};
		}

	public:
		def_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{
					  ast_node_type::def_t,
					  text,
					  std::move(location),
					  typename ast_node_impl_base<T>::children_type{
							  std::make_move_iterator(children.begin()),
							  std::make_move_iterator(children.end() - has_guard_node(children, 1) ? 2 : 1)
					  }},
			  // This apparent use after move is safe because we are only
			  // moving out the specific elements we need on each operation.
			  body_node{get_body_node(std::move(children))},
			  guard_node{get_guard_node(std::move(children), children.size() - this->children.size() == 2)} { }

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override
		{
			auto [num_params, param_names, param_types] = [this, &state]
			{
				if (this->children.size() > 1 && ast_node_impl_base<T>::unwrap_child(this->children[1]).type == ast_node_type::arg_list_t)
				{
					return std::make_tuple(
							ast_node_impl_base<T>::unwrap_child(this->children[1]).children.size(),
							arg_list_ast_node<T>::get_arg_names(ast_node_impl_base<T>::unwrap_child(this->children[1])),
							arg_list_ast_node<T>::get_arg_types(ast_node_impl_base<T>::unwrap_child(this->children[1]), state));
				}
				return {0, {}, {}};
			}();

			auto engine = std::ref(*state);

			auto guard = [this, engine, num_params, &param_names]
			{
				if (guard_node)
				{
					return kits::make_dynamic_proxy_function(
							[this, engine, &param_names](const kits::function_parameters& params) { return detail::eval_function(engine, *guard_node, params, param_names); },
							static_cast<kits::proxy_function_base::arity_size_type>(num_params),
							guard_node);
				}
				return {};
			}();

			try
			{
				const auto& name = ast_node_impl_base<T>::unwrap_child(this->children.front()).text;
				state->add_function(
						name,
						kits::make_dynamic_proxy_function(
								[this, engine, &param_names](const kits::function_parameters& params) { return detail::eval_function(engine, *body_node, params, param_names); },
								static_cast<kits::proxy_function_base::arity_size_type>(num_params),
								param_types,
								guard));
			}
			catch (const name_conflict_error& e) { throw eval_error{std_format::format("Function redefined '{}'", e.which())}; }

			return kits::void_var();
		}
	};

	template<typename T>
	struct method_ast_node final : ast_node_impl_base<T>
	{
		using shared_node_type = std::shared_ptr<ast_node_impl_base<T>>;

		shared_node_type body_node;
		shared_node_type guard_node;

		method_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{
					  ast_node_type::method_t,
					  text,
					  std::move(location),
					  typename ast_node_impl_base<T>::children_type{
							  std::make_move_iterator(children.begin()),
							  std::make_move_iterator(children.end() - def_ast_node<T>::has_guard_node(children, 1) ? 2 : 1)
					  }
			  },
			  // This apparent use after move is safe because we are only
			  // moving out the specific elements we need on each operation.
			  body_node{def_ast_node<T>::get_body_node(std::move(children))},
			  guard_node{def_ast_node<T>::get_guard_node(std::move(children), children.size() - this->children.size() == 2)} {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override
		{
			// The first param of a method is always the implied this ptr.
			std::vector<std::string_view> param_names{object_self_name::value};

			// we maybe need modify param_types
			// auto [param_types] = [this, &state, &param_names] 
			// {
			// 	auto args = arg_list_ast_node<T>::get_arg_names(ast_node_impl_base<T>::unwrap_child(this->children[2]));
			// 	param_names.insert(param_names.end(), args.begin(), args.end());
			// 	return arg_list_ast_node<T>::get_arg_types(ast_node_impl_base<T>::unwrap_child(this->children[2]), state);
			// }();
			{
				auto args = arg_list_ast_node<T>::get_arg_names(ast_node_impl_base<T>::unwrap_child(this->children[2]));
				param_names.insert(param_names.end(), args.begin(), args.end());
			}
			auto param_types = arg_list_ast_node<T>::get_arg_types(ast_node_impl_base<T>::unwrap_child(this->children[2]), state);

			const auto num_params = param_names.size();

			auto engine = std::ref(*state);

			auto guard = [this, engine, num_params, &param_names]
			{
				if (guard_node)
				{
					return kits::make_dynamic_proxy_function(
							[this, engine, &param_names](const kits::function_parameters& params) { return detail::eval_function(engine, *guard_node, params, param_names); },
							static_cast<kits::proxy_function_base::arity_size_type>(num_params),
							guard_node);
				}
				return {};
			}();

			try
			{
				const auto& class_name = ast_node_impl_base<T>::unwrap_child(this->children[0]).text;
				const auto& function_name = ast_node_impl_base<T>::unwrap_child(this->children[1]).text;

				if (function_name == class_name)
				{
					// constructor
					param_types.push_front(class_name, {});

					state->add_function(
							function_name,
							std::make_shared<kits::dynamic_object_constructor>(
									class_name,
									kits::make_dynamic_proxy_function(
											[this, engine, &param_names](const kits::function_parameters& params) { return detail::eval_function(engine, *body_node, params, param_names); },
											static_cast<kits::proxy_function_base::arity_size_type>(num_params),
											body_node,
											param_types,
											guard)));
				}
				else
				{
					// if the type is unknown, then this generates a function that looks up the type
					// at runtime. Defining the type first before this is called is better
					auto type = state->get_type_info(class_name, false);
					param_types.push_front(class_name, type);

					state->add_function(
							function_name,
							std::make_shared<kits::dynamic_object_function>(
									class_name,
									kits::make_dynamic_proxy_function(
											[this, engine, &param_names](const kits::function_parameters& params) { return detail::eval_function(engine, *body_node, params, param_names); },
											static_cast<kits::proxy_function_base::arity_size_type>(num_params),
											body_node,
											param_types,
											guard)));
				}
			}
			catch (const name_conflict_error& e) { throw eval_error{std_format::format("Method redefined '{}'", e.which())}; }

			return kits::void_var();
		}
	};

	template<typename T>
	struct attribute_decl_ast_node final : ast_node_impl_base<T>
	{
		attribute_decl_ast_node(
				const std::string_view text,
				parse_location location,
				typename ast_node_impl_base<T>::children_type children)
			: ast_node_impl_base{ast_node_type::attribute_decl_t, text, std::move(location), std::move(children)} {}

		kits::boxed_value do_eval(const typename ast_node_impl_base<T>::dispatch_state& state) const override
		{
			const auto& class_name = ast_node_impl_base<T>::unwrap_child(this->children.front()).text;

			try
			{
				const auto& attribute_name = ast_node_impl_base<T>::unwrap_child(this->children[1]).text;

				state->add_function(attribute_name,
				                    std::make_shared<kits::dynamic_object_function>(
						                    class_name,
						                    kits::fun([attribute_name](kits::dynamic_object& object) { return object.get_attribute(attribute_name); }),
						                    true));
			}
			catch (const name_conflict_error& e) { throw eval_error{std_format::format("Attribute redefined '{}'", e.which())}; }

			return kits::void_var();
		}
	};
}

#endif // GAL_LANG_KITS_EVAL_HPP
