#pragma once

#ifndef GAL_LANG_LANGUAGE_EVAL_HPP
#define GAL_LANG_LANGUAGE_EVAL_HPP

#include <gal/boxed_cast.hpp>
#include <gal/foundation/dispatcher.hpp>
#include <gal/language/common.hpp>
#include <gal/foundation/dynamic_object_function.hpp>

namespace gal::lang
{
	/**
	 * @brief Helper function that will set up the scope around a function call, including handling the named function parameters.
	 */
	namespace eval_detail
	{
		template<typename T>
		[[nodiscard]] foundation::boxed_value eval_function(
				foundation::dispatcher_detail::dispatcher& dispatcher,
				const lang::ast_node<T>& node,
				const foundation::parameters_view_type params,
				const std::span<std::string_view> param_names,
				const std::span<foundation::dispatcher_detail::engine_stack::scope_type> locals = {},
				const bool is_this_capture = false)
		{
			gal_assert(params.size() == param_names.size());

			foundation::dispatcher_detail::dispatcher_state state{dispatcher};

			const auto* object_this = [params, &state]
			{
				auto& scope = state.stack()->recent_scope();
				if (const auto it = scope.find(lang::object_self_type_name::value);
					it != scope.end()) { return &it->second; }
				if (not params.empty()) { return &params.front(); }
				return nullptr;
			}();

			foundation::dispatcher_detail::scoped_stack_scope scoped_stack{state};
			if (object_this && not is_this_capture) { state.add_object_no_check(lang::object_self_name::value, *object_this); }

			std::ranges::for_each(
					locals,
					[&state](const auto& pair) { (void)state.add_object_no_check(pair.first, pair.second); });

			utils::zip_invoke(
					[&state](const auto& name, const auto& object) { if (name != lang::object_self_name::value) { state.add_object_no_check(name, object); } },
					param_names,
					params.begin());

			try { return node.eval(state); }
			catch (interrupt_type::return_value& ret) { return std::move(ret.value); }
		}

		[[nodiscard]] inline foundation::boxed_value clone_if_necessary(
				foundation::boxed_value incoming,
				foundation::dispatcher_detail::dispatcher::function_cache_location_type& location,
				const foundation::dispatcher_detail::dispatcher_state& state)
		{
			if (not incoming.is_xvalue())
			{
				if (const auto& ti = incoming.type_info(); ti.is_arithmetic()) { return foundation::boxed_number::clone(incoming); }
				else
				{
					if (ti.bare_equal(typeid(bool))) { return foundation::boxed_value{*static_cast<const bool*>(incoming.get_const_raw())}; }
					if (ti.bare_equal(typeid(foundation::string_type))) { return foundation::boxed_value{*static_cast<const foundation::string_type*>(incoming.get_const_raw())}; }
				}
				return state->call_function(lang::object_clone_interface_name::value, location, foundation::parameters_view_type{incoming}, state.conversion());
			}
			incoming.to_lvalue();
			return incoming;
		}
	}

	namespace lang
	{
		template<typename T>
		struct id_ast_node final : ast_node<T>
		{
		private:
			mutable foundation::dispatcher_detail::dispatcher::variable_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				try { return state.get_object(this->text, location_); }
				catch (std::exception&) { throw exception::eval_error{std_format::format("Can not find object '{}'", this->text)}; }
			}

		public:
			GAL_AST_SET_RTTI(id_ast_node<T>)

			id_ast_node(const foundation::string_view_type text, parse_location location)
				: ast_node{get_rtti_index(), text, std::move(location)} {}
		};

		template<typename T>
		struct constant_ast_node final : ast_node<T>
		{
			foundation::boxed_value value;

		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override { return value; }

		public:
			GAL_AST_SET_RTTI(constant_ast_node<T>)

			constant_ast_node(
					const foundation::string_view_type text,
					parse_location location,
					foundation::boxed_value value)
				: ast_node{get_rtti_index(), text, std::move(location)},
				  value{std::move(value)} {}

			explicit constant_ast_node(foundation::boxed_value value)
				: constant_ast_node({}, {}, std::move(value)) {}
		};

		template<typename T>
		struct reference_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override { return state.add_object_no_check(this->front().text, foundation::boxed_value{}); }

		public:
			GAL_AST_SET_RTTI(reference_ast_node<T>)

			reference_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{ast_node_type::reference_t, text, std::move(location), std::move(children)} { gal_assert(this->size() == 1); }
		};

		template<typename T>
		struct compiled_ast_node final : ast_node<T>
		{
			using original_node_type = ast_node_ptr<T>;
			using function_type = std::function<foundation::boxed_value(const typename ast_node<T>::children_type&, const foundation::dispatcher_detail::dispatcher_state&)>;

			original_node_type original_node;
			function_type function;

		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override { return function(this->children_, state); }

		public:
			GAL_AST_SET_RTTI(compiled_ast_node<T>)

			compiled_ast_node(
					original_node_type original_node,
					typename ast_node<T>::children_type children,
					function_type function)
				: ast_node{
						  get_rtti_index(),
						  original_node->text,
						  original_node->location,
						  std::move(children)},
				  original_node{std::move(original_node)},
				  function{std::move(function)} {}
		};

		template<typename T>
		struct unary_operator_ast_node final : ast_node<T>
		{
		private:
			algebraic_operations operation_;

			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				const foundation::boxed_value object{this->front().eval(state)};

				try
				{
					// short circuit arithmetic operations
					if (operation_ != algebraic_operations::unknown && operation_ != algebraic_operations::bitwise_and &&
					    object.type_info().is_arithmetic()) { return foundation::boxed_number::unary_invoke(object, operation_); }

					const foundation::dispatcher_detail::scoped_function_scope scoped_function{state};

					const foundation::parameters_view_type params{object};
					scoped_function.push_params(params);
					(void)state->call_function(this->text, location_, params, state.conversion());
				}
				catch (const exception::dispatch_error& e) { throw exception::eval_error{std_format::format("Error with unary operator '{}' evaluation", this->text), e.parameters, e.functions, false, *state}; }

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(unary_operator_ast_node<T>)

			unary_operator_ast_node(
					const algebraic_operation_name_type operation,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), operation, std::move(location), std::move(children)},
				  operation_{algebraic_operation(operation, true)} {}
		};

		template<typename T>
		struct fold_right_binary_operator_ast_node final : ast_node<T>
		{
		private:
			algebraic_operations operation_;
			foundation::boxed_value rhs_;

			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_{};

			foundation::boxed_value do_operation(
					const foundation::dispatcher_detail::dispatcher_state& state,
					const algebraic_operation_name_type operation,
					const foundation::boxed_value& lhs) const
			{
				try
				{
					if (lhs.type_info().is_arithmetic())
					{
						// If it's an arithmetic operation we want to short circuit dispatch
						try
						{
							return foundation::boxed_number::binary_invoke(
									operation_,
									lhs,
									rhs_);
						}
						catch (const exception::arithmetic_error&) { throw; }
						catch (...) { throw exception::eval_error{std_format::format("Error with numeric operator '{}' called", operation)}; }
					}

					const foundation::dispatcher_detail::scoped_function_scope function_scope{state};

					const auto params = foundation::parameters_view_type{lhs, rhs_};

					function_scope.push_params(params);
					return state->call_function(operation, location_, params, state.conversion());
				}
				catch (const exception::dispatch_error& e)
				{
					throw exception::eval_error
					{
							std_format::format("Can not find appropriate '{}' operator", operation_),
							e.parameters,
							e.functions,
							false,
							*state};
				}
			}

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override { return do_operation(state, this->text, this->front().eval(state)); }

		public:
			GAL_AST_SET_RTTI(fold_right_binary_operator_ast_node<T>)

			fold_right_binary_operator_ast_node(
					const algebraic_operation_name_type operation,
					parse_location location,
					typename ast_node<T>::children_type children,
					foundation::boxed_value rhs)
				: ast_node{get_rtti_index(), operation, std::move(location), std::move(children)},
				  operation_{algebraic_operation(operation)},
				  rhs_{std::move(rhs)} {}
		};

		template<typename T>
		struct binary_operator_ast_node final : ast_node<T>
		{
		private:
			algebraic_operations operation_;

			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_{};

			foundation::boxed_value do_operation(

					const foundation::dispatcher_detail::dispatcher_state& state,
					const algebraic_operations operation,
					const algebraic_operation_name_type operation_string,
					const foundation::boxed_value& lhs,
					const foundation::boxed_value& rhs) const
			{
				try
				{
					if (operation != algebraic_operations::unknown && lhs.type_info().is_arithmetic() && rhs.type_info().is_arithmetic())
					{
						// If it's an arithmetic operation we want to short circuit dispatch
						try { return foundation::boxed_number::binary_invoke(operation, lhs, rhs); }
						catch (const exception::arithmetic_error&) { throw; }
						catch (...) { throw exception::eval_error{std_format::format("Error with numeric operator '{}' called", operation_string)}; }
					}

					const foundation::dispatcher_detail::scoped_function_scope function_scope{state};

					const auto params = foundation::parameters_view_type{lhs, rhs};

					function_scope.push_params(params);
					return state->call_function(operation_string, location_, params, state.conversion());
				}
				catch (const exception::dispatch_error& e) { throw exception::eval_error{std_format::format("Can not find appropriate '{}' operator", operation_string), e.parameters, e.functions, false, *state}; }
			}

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override { return do_operation(state, operation_, this->text, this->get_child(0).eval(state), this->get_child(1).eval(state)); }

		public:
			GAL_AST_SET_RTTI(binary_operator_ast_node<T>)

			binary_operator_ast_node(
					const algebraic_operation_name_type operation,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), operation, std::move(location), std::move(children)},
				  operation_{algebraic_operation(operation)} {}
		};

		template<typename T>
		struct fun_call_ast_node : ast_node<T>
		{
			template<typename>
			friend struct unused_return_fun_call_ast_node;

		private:
			template<bool SaveParams>
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const
			{
				const foundation::dispatcher_detail::scoped_function_scope function_scope{state};

				foundation::parameters_type params;
				params.reserve(this->get_child(1).size());

				std::ranges::for_each(
						this->get_child(1),
						[&params, &state](const auto& child) { params.push_back(child.eval(state)); });

				if constexpr (SaveParams) { function_scope.push_params(foundation::parameters_view_type{params}); }
				else { }

				const foundation::boxed_value function{this->front().eval(state)};

				try { return (*state->boxed_cast<const foundation::proxy_function_base*>(function))(foundation::parameters_view_type{params}, state.conversion()); }
				catch (const exception::dispatch_error& e) { throw exception::eval_error{std_format::format("{} with function '{}' called", e.what(), this->front()).text, e.parameters, e.functions, false, *state}; }
				catch (const exception::bad_boxed_cast&)
				{
					try
					{
						// handle the case where there is only 1 function to try to call and dispatch fails on it
						throw exception::eval_error{
								std_format::format("Error with function '{}' called", this->front().text),
								params,
								foundation::proxy_functions_view_type{state->boxed_cast<const foundation::proxy_function>(function)},
								false,
								*state};
					}
					catch (const exception::bad_boxed_cast&)
					{
						throw exception::eval_error{
								std_format::format("'{}' does not evaluate to a function", this->front().pretty_print())};
					}
				}
				catch (const exception::arity_error& e)
				{
					throw exception::eval_error{
							std_format::format("{} with function '{}' called", e.what(), this->front().text)};
				}
				catch (const exception::guard_error& e)
				{
					throw exception::eval_error{
							std_format::format("{} with function '{}' called", e.what(), this->front().text)};
				}
				catch (interrupt_type::return_value& ret) { return std::move(ret.value); }
			}

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override { return do_eval<true>(state); }

		public:
			GAL_AST_SET_RTTI(fun_call_ast_node<T>)

			fun_call_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} { gal_assert(not this->empty()); }
		};

		template<typename T>
		struct unused_return_fun_call_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override { return fun_call_ast_node<T>::do_eval < false > (state); }

		public:
			GAL_AST_SET_RTTI(unused_return_fun_call_ast_node<T>)

			unused_return_fun_call_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} { gal_assert(not this->empty()); }
		};

		template<typename T>
		struct array_call_ast_node final : ast_node<T>
		{
		private:
			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				const foundation::dispatcher_detail::scoped_function_scope scoped_function{state};

				const foundation::parameters_view_type params{this->get_child(0).eval(state), this->get_child(1).eval(state)};

				try
				{
					scoped_function.push_params(params);
					return state->call_function(container_subscript_interface_name::value, location_, params, state.conversion());
				}
				catch (const exception::dispatch_error& e) { throw exception::eval_error{std_format::format("Can not find appropriate array lookup operator '{}'", container_subscript_interface_name::value), e.parameters, e.functions, false, *state}; }
			}

		public:
			GAL_AST_SET_RTTI(array_call_ast_node<T>)

			array_call_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct dot_access_ast_node final : ast_node<T>
		{
		private:
			std::string_view function_name_;

			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_{};
			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type array_location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				const foundation::dispatcher_detail::scoped_function_scope scoped_function{state};

				auto ret = this->front().eval(state);

				foundation::parameters_type params{ret};

				const bool has_function_params = [this, &params, &state]
				{
					if (this->get_child(1).size() > 1)
					{
						std::ranges::for_each(
								this->get_child(1).get_child(1),
								[&params, &state](const auto& c) { params.push_back(c.eval(state)); });

						return true;
					}
					return false;
				}();

				const foundation::parameters_view_type ps{params};

				scoped_function.push_params(ps);

				try { ret = state->call_member_function(function_name_, location_, ps, has_function_params, state.conversion()); }
				catch (const exception::dispatch_error& e)
				{
					if (e.functions.empty()) { throw exception::eval_error{std_format::format("'{}' is not a function")}; }
					throw exception::eval_error{std_format::format("{} for function '{}' called", e.what(), function_name_), e.parameters, e.functions, true, *state};
				}
				catch (interrupt_type::return_value& r) { ret = std::move(r.value); }

				if (const auto c = this->get_child(1); c.type == ast_node_type::array_call_t)
				{
					try
					{
						const foundation::parameters_view_type p{ret, c.get_child(1).eval(state)};
						ret = state->call_function(container_subscript_interface_name::value, array_location_, p, state.conversion());
					}
					catch (const exception::dispatch_error& e) { throw exception::eval_error{std_format::format("Can not find appropriate array lookup operator '{}'", container_subscript_interface_name::value), e.parameters, e.functions, false, *state}; }
				}

				return ret;
			}

		public:
			GAL_AST_SET_RTTI(dot_access_ast_node<T>)

			dot_access_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)},
				  function_name_{
						  this->get_child(1).template is_any<fun_call_ast_node<T>, array_call_ast_node<T>>() ? this->get_child(1).front().text : this->get_child(1).text} {}
		};

		template<typename T>
		struct arg_ast_node final : ast_node<T>
		{
			GAL_AST_SET_RTTI(arg_ast_node<T>)

			arg_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct arg_list_ast_node final : ast_node<T>
		{
			GAL_AST_SET_RTTI(arg_list_ast_node<T>)

			arg_list_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}

			static std::string_view get_arg_name(const ast_node<T>& node)
			{
				if (node.empty()) { return node.text; }
				if (node.size() == 1) { return node.front().text; }
				return node.get_child(1).text;
			}

			static std::vector<std::string_view> get_arg_names(const ast_node<T>& node)
			{
				std::vector<decltype(get_arg_name(node))> ret;
				ret.reserve(node.size());

				std::ranges::for_each(
						node,
						[&ret](const auto& child) { ret.push_back(get_arg_name(child)); });

				return ret;
			}

			static std::pair<std::string_view, foundation::gal_type_info> get_arg_type(const ast_node<T>& node, const foundation::dispatcher_detail::dispatcher_state& state)
			{
				if (node.size() < 2) { return {}; }
				return {node.front().text, state->get_type_info(node.front().text, false)};
			}

			static foundation::parameter_type_mapper get_arg_types(const ast_node<T>& node, const foundation::dispatcher_detail::dispatcher_state& state)
			{
				std::vector<decltype(get_arg_type(node, state))> ret;
				ret.reserve(node.size());

				std::ranges::for_each(
						node,
						[&ret, &state](const auto& child) { ret.push_back(get_arg_type(child, state)); });

				return foundation::parameter_type_mapper{ret};
			}
		};

		template<typename T>
		struct equation_ast_node final : ast_node<T>
		{
		private:
			algebraic_operations operation_;

			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_{};
			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type clone_location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				foundation::dispatcher_detail::scoped_function_scope function_scope{state};

				auto params = [&]
				{
					// The RHS *must* be evaluated before the LHS
					// consider `var range = range(x)`
					// if we declare the variable in scope first, then the name lookup fails
					// for the RHS
					auto rhs = this->get_child(1).eval(state);
					auto lhs = this->get_child(0).eval(state);
					return foundation::parameters_type{std::move(lhs), std::move(rhs)};
				}();

				if (params[0].is_xvalue()) { throw exception::eval_error{"Error, can not assign to a temporary value"}; }
				if (params[0].is_const()) { throw exception::eval_error{"Error, can not assign to a immutable value"}; }

				if (operation_ != algebraic_operations::unknown && params[0].type_info().is_arithmetic() && params[1].type_info().is_arithmetic())
				{
					try { return foundation::boxed_number::binary_invoke(operation_, params[0], params[1]); }
					catch (const std::exception&) { throw exception::eval_error{"Error with unsupported arithmetic assignment operation"}; }
				}
				if (operation_ == algebraic_operations::assign)
				{
					try
					{
						if (params[0].is_undefined())
						{
							if (not this->empty() &&
							    (this->front().template is<reference_ast_node<T>>() ||
							     (not this->front().empty() && this->front().front().template is<reference_ast_node<T>>())))
							{
								// todo: This does not handle the case of an unassigned reference variable being assigned outside of its declaration
								params[0].assign(params[1]).reset_return_value();
								return params[0];
							}
							params[1] = eval_detail::clone_if_necessary(std::move(params[1]), clone_location_, state);
						}

						try { return state->call_function(this->text, location_, params, state.conversion()); }
						catch (const exception::dispatch_error& e)
						{
							throw exception::eval_error{
									std_format::format("Can not find appropriate '{}' operator", this->text),
									e.parameters,
									e.functions,
									false,
									*state};
						}
					}
					catch (const exception::dispatch_error& e)
					{
						throw exception::eval_error{
								"Missing clone or copy constructor for right hand side of equation",
								e.parameters,
								e.functions,
								false,
								*state};
					}
				}
				if (this->text == operator_assign_if_type_match_name::value)
				{
					if (params[0].is_undefined() || foundation::boxed_value::is_type_matched(params[0], params[1]))
					{
						params[0].assign(params[1]).to_lvalue();
						return params[0];
					}
					throw exception::eval_error{"Mismatched types in equation"};
				}

				try { return state->call_function(this->text, location_, params, state.conversion()); }
				catch (const exception::dispatch_error& e)
				{
					throw exception::eval_error{
							std_format::format("Can not find appropriate '{}' operator", this->text),
							e.parameters,
							e.functions,
							false,
							*state};
				}
			}

		public:
			GAL_AST_SET_RTTI(equation_ast_node<T>)

			equation_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)},
				  operation_{algebraic_operation(this->text)} { gal_assert(this->size() == 2); }
		};

		template<typename T>
		struct global_decl_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				const auto name = [&]() -> std::string_view
				{
					if (this->front().template is<reference_ast_node<T>>()) { return this->front().front().text; }
					return this->front().text;
				}();

				return state->add_global_mutable_no_throw(name, {});
			}

		public:
			GAL_AST_SET_RTTI(global_decl_ast_node<T>)

			global_decl_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct var_decl_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				const auto& name = this->front().text;

				try { return state.add_object_no_check(name, foundation::boxed_value{}); }
				catch (const exception::name_conflict_error& e) { throw exception::eval_error{std_format::format("Variable redefined '{}'", e.which())}; }
			}

		public:
			GAL_AST_SET_RTTI(var_decl_ast_node<T>)

			var_decl_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct assign_decl_ast_node final : ast_node<T>
		{
		private:
			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				const auto& name = this->front().text;

				try
				{
					auto object = eval_detail::clone_if_necessary(this->get_child(1).eval(state), location_, state);
					object.to_lvalue();
					state.add_object_no_check(name, object);
					return object;
				}
				catch (const exception::name_conflict_error& e) { throw exception::eval_error{std_format::format("Variable redefined '{}'", e.which())}; }
			}

		public:
			GAL_AST_SET_RTTI(assign_decl_ast_node<T>)

			assign_decl_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct class_decl_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				foundation::dispatcher_detail::scoped_scope scoped_scope{state};

				// todo: do this better
				// todo: name?
				// put class name in current scope, so it can be looked up by the attrs and methods
				state.add_object_no_check("_current_class_name", const_var(this->front().text));

				this->get_child(1).eval(state);

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(class_decl_ast_node<T>)

			class_decl_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		/**
	 * @brief member definition ::=
	 *		keyword_member_name::value class_name keyword_class_scope_name::value member_name (or 'decl' class_name '::' member_name)
	 *		keyword_member_name::value member_name (or 'decl' member_name)(must in class)
	 *
	 * @code
	 *
	 * decl my_class::a
	 * decl my_class::b
	 *
	 * class my_class
	 * {
	 *	decl a
	 *	decl b
	 * }
	 *
	 * @endcode 
	 */
		template<typename T>
		struct member_decl_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				const auto& class_name = this->get_child(0).text;

				try
				{
					const auto& member_name = this->get_child(1).text;

					state->add_function(member_name,
					                    std::make_shared<foundation::dynamic_object_function>(
							                    class_name,
							                    fun([member_name](foundation::dynamic_object& object) { return object.get_member(member_name); }),
							                    true));
				}
				catch (const exception::name_conflict_error& e) { throw exception::eval_error{std_format::format("Member redefined '{}'", e.which())}; }

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(member_decl_ast_node<T>)

			member_decl_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		/**
	 * @brief 
	 * function definition ::=
	 *	keyword_define_name::value identifier keyword_function_parameter_bracket_name::left_type::value [[type] arg...] keyword_function_parameter_bracket_name::right_type::value [keyword_set_guard_name::value guard] keyword_block_begin_name::value block
	 *	(or 'def' identifier '(' [type] arg... ')' ['expect' guard] ':' block)
	 *
	 *	method definition ::=
	 * keyword_define_name::value class_name keyword_class_scope_name::value method_name keyword_function_parameter_bracket_name::left_type::value [[type] arg...] keyword_function_parameter_bracket_name::right_type::value [keyword_set_guard_name::value guard] keyword_block_begin_name::value block
	 * (or 'def' class_name '::' method_name '(' [type] arg... ')' ['expect' guard] ':' block)
	 * keyword_define_name::value method_name keyword_function_parameter_bracket_name::left_type::value [[type] arg...] keyword_function_parameter_bracket_name::right_type::value [keyword_set_guard_name::value guard] keyword_block_begin_name::value block
	 * (or 'def' method_name '(' [type] arg... ')' ['expect' guard] keyword_block_begin_name::value block)(must in class)
	 *
	 * @code
	 *
	 * # function
	 * def my_func(arg1, arg2) expect arg1 != 42:
	 *	print("arg1 not equal 42")
	 *
	 * # method
	 * def my_class::func(arg1, arg2) expect arg1 != 42:
	 *	print("arg1 not equal 42")
	 *
	 * class my_class
	 * {
	 *	def func(arg1, arg2) expect arg1 != 42:
	 *		print("arg1 not equal 42")
	 * }
	 *
	 * @endcode 
	 */
		template<typename T>
		struct def_ast_node final : ast_node<T>
		{
			using shared_node_type = std::shared_ptr<ast_node<T>>;

			shared_node_type body_node;
			shared_node_type guard_node;

			[[nodiscard]] static shared_node_type get_body_node(typename ast_node<T>::children_type&& children) { return std::move(children.back()); }

			[[nodiscard]] static bool has_guard_node(const typename ast_node<T>::children_type& children, const typename ast_node<T>::children_type::size_type offset) noexcept
			{
				if (children.size() > offset + 2)
				{
					if (children[offset + 1]->type != ast_node_type::arg_list_t) { return true; }
					if (children.size() > offset + 3) { return true; }
				}
				return false;
			}

			[[nodiscard]] static shared_node_type get_guard_node(typename ast_node<T>::children_type&& children, const bool has_guard)
			{
				if (has_guard) { return std::move(*(children.end() - 2)); }
				return {};
			}

		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				auto [num_params, param_names, param_types] = [this, &state]
				{
					if (this->size() > 1 && this->get_child(1).template is<arg_list_ast_node<T>>())
					{
						return std::make_tuple(
								this->get_child(1).size(),
								arg_list_ast_node<T>::get_arg_names(this->get_child(1)),
								arg_list_ast_node<T>::get_arg_types(this->get_child(1), state));
					}
					return {0, {}, {}};
				}();

				auto dispatcher = std::ref(*state);

				auto guard = [this, dispatcher, num_params, &param_names]
				{
					if (guard_node)
					{
						return foundation::make_dynamic_proxy_function(
								[this, dispatcher, &param_names](const foundation::parameters_view_type params) { return eval_detail::eval_function(dispatcher, *guard_node, params, param_names); },
								static_cast<foundation::proxy_function_base::arity_size_type>(num_params),
								guard_node);
					}
					return {};
				}();

				try
				{
					const auto& name = this->front().text;
					state->add_function(
							name,
							foundation::make_dynamic_proxy_function(
									[this, dispatcher, &param_names](const foundation::parameters_view_type params) { return eval_detail::eval_function(dispatcher, *body_node, params, param_names); },
									static_cast<foundation::proxy_function_base::arity_size_type>(num_params),
									param_types,
									guard));
				}
				catch (const exception::name_conflict_error& e) { throw exception::eval_error{std_format::format("Function redefined '{}'", e.which())}; }

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(def_ast_node<T>)

			def_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{
						  get_rtti_index(),
						  text,
						  std::move(location),
						  typename ast_node<T>::children_type{
								  std::make_move_iterator(children.begin()),
								  std::make_move_iterator(children.end() - has_guard_node(children, 1) ? 2 : 1)}},
				  // This apparent use after move is safe because we are only
				  // moving out the specific elements we need on each operation.
				  body_node{get_body_node(std::move(children))},
				  guard_node{get_guard_node(std::move(children), children.size() - this->size() == 2)} { }
		};

		template<typename T>
		struct method_ast_node final : ast_node<T>
		{
			using shared_node_type = std::shared_ptr<ast_node<T>>;

			shared_node_type body_node;
			shared_node_type guard_node;

		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				// The first param of a method is always the implied this ptr.
				std::vector<std::string_view> param_names{object_self_name::value};

				// we maybe need modify param_types
				// auto [param_types] = [this, &state, &param_names]
				// {
				// 	auto args = arg_list_ast_node<T>::get_arg_names(this->get_child(2));
				// 	param_names.insert(param_names.end(), args.begin(), args.end());
				// 	return arg_list_ast_node<T>::get_arg_types(this->get_child(2)), state);
				// }();
				{
					auto args = arg_list_ast_node<T>::get_arg_names(this->get_child(2));
					param_names.insert(param_names.end(), args.begin(), args.end());
				}
				auto param_types = arg_list_ast_node<T>::get_arg_types(this->get_child(2), state);

				const auto num_params = param_names.size();

				auto dispatcher = std::ref(*state);

				auto guard = [this, dispatcher, num_params, &param_names]
				{
					if (guard_node)
					{
						return foundation::make_dynamic_proxy_function(
								[this, dispatcher, &param_names](const foundation::parameters_view_type params) { return eval_detail::eval_function(dispatcher, *guard_node, params, param_names); },
								static_cast<foundation::proxy_function_base::arity_size_type>(num_params),
								guard_node);
					}
					return {};
				}();

				try
				{
					const auto& class_name = this->get_child(0).text;
					const auto& function_name = this->get_child(1).text;

					if (function_name == class_name)
					{
						// constructor
						param_types.push_front(class_name, {});

						state->add_function(
								function_name,
								std::make_shared<foundation::dynamic_object_constructor>(
										class_name,
										foundation::make_dynamic_proxy_function(
												[this, dispatcher, &param_names](const foundation::parameters_view_type params) { return eval_detail::eval_function(dispatcher, *body_node, params, param_names); },
												static_cast<foundation::proxy_function_base::arity_size_type>(num_params),
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
								std::make_shared<foundation::dynamic_object_function>(
										class_name,
										foundation::make_dynamic_proxy_function(
												[this, dispatcher, &param_names](const foundation::parameters_view_type params) { return eval_detail::eval_function(dispatcher, *body_node, params, param_names); },
												static_cast<foundation::proxy_function_base::arity_size_type>(num_params),
												body_node,
												param_types,
												guard)));
					}
				}
				catch (const exception::name_conflict_error& e) { throw exception::eval_error{std_format::format("Method redefined '{}'", e.which())}; }

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(method_ast_node<T>)

			method_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{
						  get_rtti_index(),
						  text,
						  std::move(location),
						  typename ast_node<T>::children_type{
								  std::make_move_iterator(children.begin()),
								  std::make_move_iterator(children.end() - def_ast_node<T>::has_guard_node(children, 1) ? 2 : 1)}},
				  // This apparent use after move is safe because we are only
				  // moving out the specific elements we need on each operation.
				  body_node{def_ast_node<T>::get_body_node(std::move(children))},
				  guard_node{def_ast_node<T>::get_guard_node(std::move(children), children.size() - this->size() == 2)} { }
		};

		template<typename T>
		struct lambda_ast_node final : ast_node<T>
		{
			using shared_node_type = std::shared_ptr<ast_node<T>>;

		private:
			decltype(arg_list_ast_node<T>::get_arg_names(std::declval<ast_node<T>>()->get_child(1))) param_names_;
			shared_node_type lambda_node_;

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				const auto& [captures, is_capture_this] = [&]
				{
					foundation::dispatcher_detail::engine_stack::scope_type named_captures;
					bool capture_this = false;

					std::ranges::for_each(
							this->front(),
							[&named_captures, &capture_this, &state](const auto& c)
							{
								const auto& cf = c.front();
								named_captures.emplace(cf.text, cf.eval(state));
								capture_this = cf.text == object_self_name::value;
							});

					return std::make_pair(std::move(named_captures), capture_this);
				}();

				const auto [num_params, param_types] = [this, &state]
				{
					const auto& params = this->get_child(1);
					return std::make_pair(params.size(), arg_list_ast_node<T>::get_arg_types(params, state));
				}();

				auto dispatcher = std::ref(*state);

				return foundation::boxed_value{
						foundation::make_dynamic_proxy_function(
								[dispatcher, this, &captures, is_capture_this](const foundation::parameters_view_type params) { return eval_detail::eval_function(dispatcher, lambda_node_, params, param_names_, captures, is_capture_this); },
								static_cast<foundation::proxy_function_base::arity_size_type>(num_params),
								lambda_node_,
								param_types)};
			}

		public:
			GAL_AST_SET_RTTI(lambda_ast_node<T>)

			lambda_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{
						  get_rtti_index(),
						  text,
						  std::move(location),
						  typename ast_node<T>::children_type{
								  std::make_move_iterator(children.begin()),
								  std::make_move_iterator(children.end() - 1)}},
				  param_names_{arg_list_ast_node<T>::get_arg_names(this->get_child(1))},
				  // This apparent use after move is safe because we are only
				  // moving out the specific elements we need on each operation.
				  lambda_node_{std::move(children.back())} { }
		};

		template<typename T>
		struct block_ast_node;

		template<typename T>
		struct no_scope_block_ast_node final : ast_node<T>
		{
			friend struct block_ast_node<T>;

		private:
			static foundation::boxed_value do_eval(const ast_node<T>& node, const foundation::dispatcher_detail::dispatcher_state& state)
			{
				std::ranges::for_each(
						node,
						[&state](const auto& c) { c.eval(state); });

				return node.back().eval(state);
			}

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override { return do_eval(*this, state); }

		public:
			GAL_AST_SET_RTTI(no_scope_block_ast_node<T>)

			no_scope_block_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct block_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				foundation::dispatcher_detail::scoped_scope scoped_scope{state};
				return no_scope_block_ast_node<T>::do_eval(*this, state);
			}

		public:
			GAL_AST_SET_RTTI(block_ast_node<T>)

			block_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		/**
		 * @brief
		 * if block ::= keyword_if_name::value condition keyword_branch_select_end_name::value block (or 'if' condition ':' block)
		 * else if block ::= keyword_else_name::value keyword_if_name::value condition keyword_branch_select_end_name::value block (or 'else if' condition ':' block)
		 * else block ::= keyword_else_name::value keyword_branch_select_end_name::value block (or 'else' ':' block)
		 *
		 * @code
		 *
		 * if 1 == 2:
		 *	print("impossible happened!")
		 * else if True:
		 *	print("of course")
		 * else:
		 *	print("impossible happened!")
		 *
		 * @endcode 
		 */
		template<typename T>
		struct if_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				if (this->get_bool_condition(this->get_child(0).eval(state), state)) { return this->get_child(1).eval(state); }
				return this->get_child(2).eval(state);
			}

		public:
			GAL_AST_SET_RTTI(if_ast_node<T>)

			if_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} { gal_assert(this->size() == 3); }
		};

		/**
		 * @brief
		 * while block ::=
		 * keyword_while_name::value condition keyword_block_begin_name::value block (or 'while' condition : block)
		 *
		 * @code
		 *
		 * var i = 42;
		 * while i != 0:
		 *	i -=1
		 *
		 * @endcode 
		 */
		template<typename T>
		struct while_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				foundation::dispatcher_detail::scoped_scope scoped_scope{state};

				try
				{
					while (this->get_scoped_bool_condition(this->get_child(0), state))
					{
						try { this->get_child(1).eval(state); }
						catch (interrupt_type::continue_loop&)
						{
							// we got a continued exception, which means all the remaining
							// loop implementation is skipped, and we just need to continue to
							// the next condition test
						}
					}
				}
				catch (interrupt_type::break_loop&)
				{
					// loop was broken intentionally
				}

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(while_ast_node<T>)

			while_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		/**
		 * @brief
		 * for block ::=
		 * keyword_for_name::value [initial] keyword_for_loop_variable_delimiter_name::value stop_condition keyword_for_loop_variable_delimiter_name loop_expression keyword_block_begin_name::value block
		 * (or 'for' [initial] ';' stop_condition ';' loop_expression ':' block)
		 *
		 * @code
		 *
		 * var i = 42;
		 * for ; i != 0; i -= 1:
		 *	# do something here
		 *
		 * for var i = 0; i < 42; i += 1:
		 *	# do something here
		 *
		 * @endcode 
		 */
		template<typename T>
		struct for_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				foundation::dispatcher_detail::scoped_scope scoped_scope{state};

				try
				{
					for (this->get_child(0).eval(state);
					     this->get_scoped_bool_condition(this->get_child(1), state);
					     this->get_child(2).eval(state))
					{
						try
						{
							// Body of Loop
							this->get_child(3).eval(state);
						}
						catch (interrupt_type::continue_loop&)
						{
							// we got a continued exception, which means all the remaining
							// loop implementation is skipped, and we just need to continue to
							// the next iteration step
						}
					}
				}
				catch (interrupt_type::break_loop&)
				{
					// loop broken
				}

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(for_ast_node<T>)

			for_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} { gal_assert(this->size() == 4); }
		};

		template<typename T>
		struct ranged_for_ast_node final : ast_node<T>
		{
		private:
			using location_type = foundation::dispatcher_detail::dispatcher::function_cache_location_type;

			location_type range_location_{};
			location_type empty_location_{};
			location_type front_location_{};
			location_type pop_front_location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				const auto get_function = [&state](const auto& name, location_type& location)
				{
					if (location.has_value()) { return *location; }

					auto func = state->get_function(name);
					location.emplace(func);
					return func;
				};

				const auto call_function = [&state](const auto& function, const auto& param) { return dispatch(*function, foundation::parameters_view_type{param}, state.conversion()); };

				const auto& loop_var_name = this->get_child(0).text;
				auto range_expression_result = this->get_child(1).eval(state);

				const auto do_loop = [&loop_var_name, this, &state](const auto& ranged)
				{
					try
					{
						std::ranges::for_each(
								ranged,
								[&loop_var_name, this, &state]<typename Var>(Var&& var)
								{
									// This scope push and pop might not be the best thing for perf,
									// but we know it's 100% correct
									foundation::dispatcher_detail::scoped_scope scoped_scope{state};
									if constexpr (std::is_same_v<Var, foundation::boxed_value>) { state.add_object_no_check(loop_var_name, std::move(var)); }
									else { state.add_object_no_check(loop_var_name, foundation::boxed_value{std::ref(var)}); }

									try { this->get_child(2).eval(state); }
									catch (interrupt_type::continue_loop&) { }
								});
					}
					catch (interrupt_type::break_loop&)
					{
						// loop broken
					}

					return void_var();
				};

				// todo: list format container type
				if (range_expression_result.type_info().bare_equal(typeid(foundation::parameters_type))) { return do_loop(boxed_cast<const foundation::parameters_type&>(range_expression_result)); }
				// todo: map format container type
				if (range_expression_result.type_info().bare_equal(typeid(foundation::dispatcher_detail::engine_stack::scope_type))) { return do_loop(boxed_cast<const foundation::dispatcher_detail::engine_stack::scope_type&>(range_expression_result)); }

				const auto range_function = get_function(container_range_interface_name::value, range_location_);
				const auto empty_function = get_function(container_empty_interface_name::value, empty_location_);
				const auto front_function = get_function(container_front_interface_name::value, front_location_);
				const auto pop_front_function = get_function(container_pop_front_interface_name::value, pop_front_location_);

				try
				{
					const auto ranged = call_function(range_function, range_expression_result);
					while (not boxed_cast<bool>(call_function(empty_function, ranged)))
					{
						foundation::dispatcher_detail::scoped_scope scoped_scope{state};

						state.add_object_no_check(loop_var_name, call_function(front_function, ranged));
						try { this->get_child(2).eval(state); }
						catch (interrupt_type::continue_loop&)
						{
							// continue statement hit
						}
						call_function(pop_front_function, ranged);
					}
				}
				catch (interrupt_type::break_loop&)
				{
					// loop broken
				}

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(ranged_for_ast_node<T>)

			ranged_for_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} { gal_assert(this->size() == 3); }
		};

		template<typename T>
		struct break_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				// todo: better way
				throw interrupt_type::break_loop{};
			}

		public:
			GAL_AST_SET_RTTI(break_ast_node<T>)

			break_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct continue_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				// todo: better way
				throw interrupt_type::continue_loop{};
			}

		public:
			GAL_AST_SET_RTTI(continue_ast_node<T>)

			continue_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct file_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				try
				{
					if (const auto size = this->size();
						size > 0)
					{
						std::ranges::for_each(
								this->begin(),
								this->end() - 1,
								[&state](const auto& child) { child.eval(state); });
						return this->back().eval(state);
					}
					return void_var();
				}
				catch (const interrupt_type::continue_loop&) { throw exception::eval_error{"Unexpected 'continue' statement outside of a loop"}; }
				catch (const interrupt_type::break_loop&) { throw exception::eval_error{"Unexpected 'break' statement outside of a loop"}; }
			}

		public:
			GAL_AST_SET_RTTI(file_ast_node<T>)

			file_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct return_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				// todo: better way
				if (not this->empty()) { throw interrupt_type::return_value{this->front().eval(state)}; }
				throw interrupt_type::return_value{void_var()};
			}

		public:
			GAL_AST_SET_RTTI(return_ast_node<T>)

			return_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{ast_node_type::return_t, text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct default_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				foundation::dispatcher_detail::scoped_scope scoped_scope{state};

				this->get_child(0).eval(state);

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(default_ast_node<T>)

			default_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} { gal_assert(this->size() == 1); }
		};

		template<typename T>
		struct case_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				foundation::dispatcher_detail::scoped_scope scoped_scope{state};

				this->get_child(1).eval(state);

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(case_ast_node<T>)

			case_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{ast_node_type::case_t, text, std::move(location), std::move(children)} { gal_assert(this->size() == 2); }
		};

		template<typename T>
		struct switch_ast_node final : ast_node<T>
		{
		private:
			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_;

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				foundation::dispatcher_detail::scoped_scope scoped_scope{state};

				const foundation::boxed_value match_value{this->front().eval(state)};

				bool breaking = false;
				decltype(this->size()) current_case = 0;
				bool has_matched = false;
				while (not breaking && current_case < this->size())
				{
					try
					{
						if (auto& current = this->get_child(current_case);
							current.template is<case_ast_node<T>>())
						{
							// This is a little odd, but because want to see both the switch and the case simultaneously, I do a downcast here.
							try
							{
								if (has_matched ||
								    boxed_cast<bool>(
										    state->call_function(operator_equal_name::value, location_, foundation::parameters_view_type{match_value, current.front().eval(state)}, state.conversion())))
								{
									current.eval(state);
									has_matched = true;
								}
							}
							catch (const exception::bad_boxed_cast&) { throw exception::eval_error{"Internal error: case guard evaluation not boolean"}; }
						}
						else if (current.template is<default_ast_node<T>>())
						{
							current.eval(state);
							has_matched = true;
						}
					}
					catch (interrupt_type::break_loop&) { breaking = true; }
					++current_case;
				}

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(switch_ast_node<T>)

			switch_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct logical_and_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				return const_var(
						this->get_bool_condition(this->get_child(0).eval(state), state) &&
						this->get_bool_condition(this->get_child(1).eval(state), state));
			}

		public:
			GAL_AST_SET_RTTI(logical_and_ast_node<T>)

			logical_and_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} { gal_assert(this->size() == 2); }
		};

		template<typename T>
		struct logical_or_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				return const_var(
						this->get_bool_condition(this->get_child(0).eval(state), state) ||
						this->get_bool_condition(this->get_child(1).eval(state), state));
			}

		public:
			GAL_AST_SET_RTTI(logical_or_ast_node<T>)

			logical_or_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} { gal_assert(this->size() == 2); }
		};

		template<typename T>
		struct inline_range_ast_node final : ast_node<T>
		{
		private:
			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				try
				{
					const auto& cs = this->front().front();

					const auto params = foundation::parameters_view_type{
							cs.get_child(0).eval(state),
							cs.get_child(1).eval(state)};

					return state->call_function(
							operator_range_generate_name::value,
							location_,
							params,
							state.conversion());
				}
				catch (const exception::dispatch_error& e) { throw exception::eval_error{std_format::format("Can not generate range vector while calling '{}'", operator_range_generate_name::value), e.parameters, e.functions, false, *state}; }
			}

		public:
			GAL_AST_SET_RTTI(inline_range_ast_node<T>)

			inline_range_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct inline_array_ast_node final : ast_node<T>
		{
		private:
			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				return onst_var(
						[this, &state]
						{
							try
							{
								// todo: container type
								foundation::parameters_type result{};

								if (not this->empty())
								{
									const auto& cs = this->front();
									result.reserve(cs.size());
									std::ranges::for_each(
											cs,
											[this, &result, &state](const auto& child) { result.push_back(eval_detail::clone_if_necessary(child.eval(state), location_, state)); });
								}

								return std::move(result);
							}
							catch (const exception::dispatch_error& e)
							{
								throw exception::eval_error{
										std_format::format("Can not find appropriate '{}' or copy constructor while insert elements into vector", object_clone_interface_name::value),
										e.parameters,
										e.functions,
										false,
										*state};
							}
						}());
			}

		public:
			GAL_AST_SET_RTTI(inline_array_ast_node<T>)

			inline_array_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct inline_map_ast_node final : ast_node<T>
		{
		private:
			mutable foundation::dispatcher_detail::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				return const_var(
						[this, &state]
						{
							try
							{
								// todo: container type
								foundation::dispatcher_detail::engine_stack::scope_type result{};

								std::ranges::for_each(
										this->front(),
										[this, &result, &state](const auto& child)
										{
											result.emplace(
													state->boxed_cast<std::string>(child.get_child(0).eval(state)),
													eval_detail::clone_if_necessary(child.get_child(1).eval(state), location_, state));
										});

								return std::move(result);
							}
							catch (const exception::dispatch_error& e)
							{
								throw exception::eval_error{
										std_format::format("Can not find appropriate '{}' or copy constructor while insert elements into map", object_clone_interface_name::value),
										e.parameters,
										e.functions,
										false,
										*state};
							}
						}());
			}

		public:
			GAL_AST_SET_RTTI(inline_map_ast_node<T>)

			inline_map_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct map_pair_ast_node final : ast_node<T>
		{
			GAL_AST_SET_RTTI(map_pair_ast_node<T>)

			map_pair_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct value_range_ast_node final : ast_node<T>
		{
			GAL_AST_SET_RTTI(value_range_ast_node<T>)

			value_range_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct catch_ast_node final : ast_node<T>
		{
			GAL_AST_SET_RTTI(catch_ast_node<T>)

			catch_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct finally_ast_node final : ast_node<T>
		{
			GAL_AST_SET_RTTI(finally_ast_node<T>)

			finally_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};

		template<typename T>
		struct try_ast_node final : ast_node<T>
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const override
			{
				auto finalize = [this, &state]
				{
					if (const auto& back = this->back();
						back.template is<finally_ast_node<T>>()) { back.front().eval(state); }
				};

				auto handle_and_finalize = [this, &state, finalize]<typename E>(const E& exception) requires(std::is_base_of_v<std::exception, E> || std::is_same_v<E, foundation::boxed_value>)
				{
					auto ret = [this, &state](const E& e)
					{
						auto end_point = this->size();
						if (this->back().template is<finally_ast_node<T>>())
						{
							gal_assert(end_point > 0);
							end_point = this->size() - 1;
						}

						return [this, &state, end_point, exception = [](const E& exc)
								{
									if constexpr (std::is_same_v<E, foundation::boxed_value>) { return exc; }
									else { return foundation::boxed_value{std::ref(exc)}; }
								}(e)]
						{
							for (decltype(this->size()) i = 1; i < end_point; ++i)
							{
								foundation::dispatcher_detail::scoped_scope scoped_scope{state};

								auto& catch_block = this->get_child(i);

								if (catch_block.size() == 1)
								{
									// no variable capture
									return catch_block.front().eval(state);
								}

								if (catch_block.size() == 2 || catch_block.size() == 3)
								{
									const auto& name = arg_list_ast_node<T>::get_arg_name(catch_block.front());

									if (foundation::parameter_type_mapper{arg_list_ast_node<T>::get_arg_type(catch_block.front())}.match(foundation::parameters_view_type{exception}, state.conversion()).first)
									{
										state.add_object_no_check(name, exception);

										if (catch_block.size() == 2)
										{
											// variable capture
											return catch_block.get_child(1).eval(state);
										}
									}

									return {};
								}

								if (const auto& back = this->back();
									back.template is<finally_ast_node<T>>()) { back.front().eval(state); }
								throw exception::eval_error{"Internal error: catch block size unrecognized"};
							}
						};
					}(exception);

					finalize();

					return ret;
				};

				foundation::dispatcher_detail::scoped_scope scoped_scope{state};

				try { return this->front().eval(state); }
				catch (const exception::eval_error& e) { return handle_and_finalize(e); }
				catch (const std::runtime_error& e) { return handle_and_finalize(e); }
				catch (const std::out_of_range& e) { return handle_and_finalize(e); }
				catch (const std::exception& e) { return handle_and_finalize(e); }
				catch (const foundation::boxed_value& e) { return handle_and_finalize(e); }
				catch (...)
				{
					finalize();
					throw;
				}
			}

		public:
			GAL_AST_SET_RTTI(try_ast_node<T>)

			try_ast_node(
					const std::string_view text,
					parse_location location,
					typename ast_node<T>::children_type children)
				: ast_node{get_rtti_index(), text, std::move(location), std::move(children)} {}
		};
	}
}

#endif // GAL_LANG_LANGUAGE_EVAL_HPP
