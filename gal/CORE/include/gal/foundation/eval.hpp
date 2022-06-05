#pragma once

#ifndef GAL_LANG_LANGUAGE_EVAL_HPP
#define GAL_LANG_LANGUAGE_EVAL_HPP

#include <gal/grammar.hpp>
#include <gal/boxed_cast.hpp>
#include <gal/foundation/dispatcher.hpp>
#include <gal/foundation/ast.hpp>
#include <gal/foundation/dynamic_function.hpp>
#include <gal/function_register.hpp>
#include <gal/foundation/boxed_exception.hpp>
#include <gal/types/range_type.hpp>
#include <gal/types/string_type.hpp>
#include <gal/types/list_type.hpp>
#include <gal/types/dict_type.hpp>

namespace gal::lang
{
	/**
	 * @brief Helper function that will set up the scope around a function call, including handling the named function parameters.
	 */
	namespace eval_detail
	{
		template<typename Range>
			requires std::is_convertible_v<std::ranges::range_value_t<Range>, ast::ast_node::identifier_type>
		[[nodiscard]] foundation::boxed_value eval_function(
				foundation::dispatcher& dispatcher,
				ast::ast_node& node,
				ast::ast_visitor_base& visitor,
				const foundation::parameters_view_type params,
				const Range& param_names,
				const foundation::engine_stack::scope_type& locals = {},
				const bool is_this_capture = false)
		{
			gal_assert(params.size() == std::ranges::size(param_names));

			foundation::dispatcher_state state{dispatcher};

			const auto* object_this = [params, &state]() -> const foundation::boxed_value*
			{
				auto& scope = state.stack().recent_scope();
				// changed since 0.5.4, see engine_stack::scope_type
				// if (const auto it = scope.find(foundation::object_self_type_name::value);
				// 	it != scope.end()) { return &it->second; }
				// new code
				if (const auto it = std::ranges::find(scope | std::views::keys, foundation::object_self_type_name::value);
					it != std::ranges::end(scope | std::views::keys)) { return &it.base()->second; }

				if (not params.empty()) { return &params.front(); }
				return nullptr;
			}();

			foundation::scoped_stack_scope scoped_stack{state};
			if (object_this && not is_this_capture) { state->add_local_or_throw(foundation::object_self_name::value, *object_this); }

			std::ranges::for_each(
					locals,
					[&state](const auto& pair) { state->add_local_or_throw(pair.first, pair.second); });

			utils::zip_invoke(
					[&state](const auto& name, const auto& object) { if (name != foundation::object_self_name::value) { state->add_local_or_throw(name, object); } },
					param_names,
					params.begin());

			try { return node.eval(state, visitor); }
			catch (interrupt_type::interrupt_return& ret) { return std::move(ret.value); }
		}

		[[nodiscard]] inline foundation::boxed_value clone_if_necessary(
				foundation::boxed_value incoming,
				foundation::dispatcher::function_cache_location_type& location,
				const foundation::dispatcher_state& state)
		{
			if (not incoming.is_xvalue())
			{
				if (const auto& ti = incoming.type_info(); ti.is_arithmetic()) { return types::number_type::clone(incoming); }
				else
				{
					if (ti.bare_equal(typeid(bool))) { return foundation::boxed_value{*static_cast<const bool*>(incoming.get_const_raw())}; }
					if (ti.bare_equal(typeid(types::string_type))) { return foundation::boxed_value{*static_cast<const types::string_type*>(incoming.get_const_raw())}; }
				}
				return state->call_function(foundation::object_clone_interface_name::value, location, foundation::parameters_view_type{incoming});
			}
			incoming.to_lvalue();
			return incoming;
		}
	}// namespace eval_detail

	namespace ast
	{
		struct noop_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state&, ast_visitor_base&) override { return void_var(); }

		public:
			GAL_AST_SET_RTTI(noop_ast_node)

			explicit noop_ast_node()
				: ast_node{get_rtti_index(), {}, parse_location{}} {}
		};

		struct id_ast_node final : ast_node
		{
		private:
			mutable foundation::dispatcher::object_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base&) override
			{
				// changed since 0.5.4, see engine_stack::scope_type
				// the following code conditions are problematic, if the reference (share) is a variable in the container, then the result is wrong (use_count is greater than 1)
				// note: see foundation::dispatcher::object_cache_location_type
				// if (
				// 	// has cache
				// 	location_.has_value() &&
				// 	// the current cache is the only reference to the target variable,
				// 	// that is, the cached referenced variable that has been invalidated
				// 	location_->use_count() == 1
				// ) { location_.reset(); }

				try { return state->get_object(this->identifier(), location_); }
				catch (std::exception&) { throw exception::eval_error{std_format::format("Can not find object '{}'", this->identifier())}; }
			}

		public:
			GAL_AST_SET_RTTI(id_ast_node)

			id_ast_node(const identifier_type identifier, const parse_location location)
				: ast_node{get_rtti_index(), identifier, location} {}
		};

		struct constant_ast_node final : ast_node
		{
			foundation::boxed_value value;

		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state&, ast_visitor_base&) override { return value; }

		public:
			GAL_AST_SET_RTTI(constant_ast_node)

			constant_ast_node(
					const identifier_type identifier,
					const parse_location location,
					foundation::boxed_value&& value)
				: ast_node{get_rtti_index(), identifier, location},
				  value{std::move(value)} {}

			explicit constant_ast_node(foundation::boxed_value&& value)
				: constant_ast_node({}, parse_location{}, std::move(value)) {}
		};

		struct reference_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base&) override
			{
				return state->add_local_or_throw(
						this->get_child(grammar::reference_ast_node::identifier_index).identifier(),
						foundation::boxed_value{});
			}

		public:
			GAL_AST_SET_RTTI(reference_ast_node)

			reference_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} { gal_assert(this->size() == 1); }
		};

		struct compiled_ast_node final : ast_node
		{
			using original_node_type = ast_node_ptr;
			using function_type = std::function<foundation::boxed_value(const children_type&, const foundation::dispatcher_state&)>;

			original_node_type original_node;
			function_type function;

		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base&) override { return function(this->children_, state); }

		public:
			GAL_AST_SET_RTTI(compiled_ast_node)

			compiled_ast_node(
					original_node_type&& original_node,
					children_type&& children,
					function_type&& function)
				: ast_node{
						  // the original_node passed to ast_node_common_base is an lvalue, so we don't need to worry about original_node being moved(changed)
						  ast_node_common_base{get_rtti_index(), *original_node},
						  std::move(children)},
				  original_node{std::move(original_node)},
				  function{std::move(function)} { }
		};

		struct unary_operator_ast_node final : ast_node
		{
		private:
			foundation::algebraic_operations operation_;

			mutable foundation::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				const foundation::boxed_value object{this->get_child(grammar::unary_operator_ast_node::index).eval(state, visitor)};

				try
				{
					// short circuit arithmetic operations
					if (operation_ != foundation::algebraic_operations::unknown && operation_ != foundation::algebraic_operations::bitwise_and &&
					    object.type_info().is_arithmetic()) { return types::number_type::unary_invoke(object, operation_); }

					const foundation::scoped_function_scope scoped_function{state};

					const foundation::parameters_view_type params{object};
					state.stack().push_params(params);
					state->call_function(this->identifier(), location_, params);
				}
				catch (const exception::dispatch_error& e)
				{
					throw exception::eval_error{
							std_format::format("Error with unary operator '{}' evaluation",
							                   this->identifier()),
							e.parameters,
							e.functions,
							false,
							*state};
				}

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(unary_operator_ast_node)

			unary_operator_ast_node(
					const foundation::algebraic_operation_name_type operation,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), operation, location, std::move(children)},
				  operation_{foundation::algebraic_operation(operation, true)} {}
		};

		struct fold_right_binary_operator_ast_node final : ast_node
		{
		private:
			foundation::algebraic_operations operation_;
			std::array<foundation::boxed_value, 2> params_;

			mutable foundation::dispatcher::function_cache_location_type location_{};

			foundation::boxed_value do_operation(
					const foundation::dispatcher_state& state,
					const foundation::algebraic_operation_name_type operation,
					const foundation::boxed_value& lhs)
			{
				try
				{
					if (lhs.type_info().is_arithmetic())
						// If it's an arithmetic operation we want to short circuit dispatch
						try
						{
							return types::number_type::binary_invoke(
									operation_,
									lhs,
									params_[1]);
						}
						catch (const exception::arithmetic_error&) { throw; }
						catch (...) { throw exception::eval_error{std_format::format("Error with numeric operator '{}' called", operation)}; }


					const foundation::scoped_function_scope function_scope{state};

					params_[0] = lhs;

					state.stack().push_params(params_);
					return state->call_function(operation, location_, params_);
				}
				catch (const exception::dispatch_error& e)
				{
					throw exception::eval_error{
							std_format::format("Can not find appropriate '{}' operator", operation),
							e.parameters,
							e.functions,
							false,
							*state};
				}
			}

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override { return do_operation(state, this->identifier(), this->get_child(grammar::fold_right_binary_operator_ast_node::lhs_index).eval(state, visitor)); }

		public:
			GAL_AST_SET_RTTI(fold_right_binary_operator_ast_node)

			fold_right_binary_operator_ast_node(
					const foundation::algebraic_operation_name_type operation,
					const parse_location location,
					children_type&& children,
					foundation::boxed_value rhs)
				: ast_node{get_rtti_index(), operation, location, std::move(children)},
				  operation_{foundation::algebraic_operation(operation)},
				  params_{{{}, std::move(rhs)}} {}
		};

		struct binary_operator_ast_node final : ast_node
		{
		private:
			foundation::algebraic_operations operation_;

			mutable foundation::dispatcher::function_cache_location_type location_{};

			foundation::boxed_value do_operation(
					const foundation::dispatcher_state& state,
					const foundation::algebraic_operations operation,
					const foundation::algebraic_operation_name_type operation_string,
					const foundation::boxed_value& lhs,
					const foundation::boxed_value& rhs) const
			{
				try
				{
					if (operation != foundation::algebraic_operations::unknown && lhs.type_info().is_arithmetic() && rhs.type_info().is_arithmetic())
					{
						// If it's an arithmetic operation we want to short circuit dispatch
						try { return types::number_type::binary_invoke(operation, lhs, rhs); }
						catch (const exception::arithmetic_error&) { throw; }
						catch (...) { throw exception::eval_error{std_format::format("Error with numeric operator '{}' called", operation_string)}; }
					}

					const foundation::scoped_function_scope function_scope{state};

					const std::array params{lhs, rhs};

					state.stack().push_params(params);
					return state->call_function(operation_string, location_, params);
				}
				catch (const exception::dispatch_error& e) { throw exception::eval_error{std_format::format("Can not find appropriate '{}' operator", operation_string), e.parameters, e.functions, false, *state}; }
			}

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				return do_operation(
						state,
						operation_,
						this->identifier(),
						this->get_child(grammar::binary_operator_ast_node::lhs_index).eval(state, visitor),
						this->get_child(grammar::binary_operator_ast_node::rhs_index).eval(state, visitor));
			}

		public:
			GAL_AST_SET_RTTI(binary_operator_ast_node)

			binary_operator_ast_node(
					const foundation::algebraic_operation_name_type operation,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), operation, location, std::move(children)},
				  operation_{foundation::algebraic_operation(operation)} {}
		};

		struct fun_call_ast_node final : ast_node
		{
			friend struct unused_return_fun_call_ast_node;

		private:
			template<bool SaveParams>
			[[nodiscard]] static foundation::boxed_value do_eval(ast_node& node, const foundation::dispatcher_state& state, ast_visitor_base& visitor)
			{
				const foundation::scoped_function_scope function_scope{state};

				foundation::parameters_type params;
				params.reserve(node.get_child(grammar::fun_call_ast_node::arg_list_index).size());

				std::ranges::for_each(
						node.get_child(grammar::fun_call_ast_node::arg_list_index).view(),
						[&params, &state, &visitor](auto& child) { params.push_back(child.eval(state, visitor)); });

				if constexpr (SaveParams) { state.stack().push_params(params); }
				else { }

				const foundation::boxed_value function{node.get_child(grammar::fun_call_ast_node::function_index).eval(state, visitor)};

				try
				{
					const foundation::convertor_manager_state convertor_manager_state{state->get_conversion_manager()};
					return (*state->boxed_cast<const foundation::function_proxy_base*>(function))(params, convertor_manager_state);
				}
				catch (const exception::dispatch_error& e)
				{
					throw exception::eval_error{
							std_format::format(
									"dispatch_error '{}' with function '{}' called.",
									e.what(),
									node.get_child(grammar::fun_call_ast_node::function_index).identifier()),
							e.parameters,
							e.functions,
							false,
							*state};
				}
				catch (const exception::bad_boxed_cast& e)
				{
					try
					{
						// handle the case where there is only 1 function to try to call and dispatch fails on it
						throw exception::eval_error{
								std_format::format(
										"bad_boxed_cast '{}' with function '{}' called.",
										e.what(),
										node.get_child(grammar::fun_call_ast_node::function_index).identifier()),
								params,
								foundation::const_function_proxies_view_type{state->boxed_cast<const foundation::const_function_proxy_type&>(function)},
								false,
								*state};
					}
					catch (const exception::bad_boxed_cast& ie)
					{
						throw exception::eval_error{
								std_format::format(
										"bad_boxed_cast '{}', '{}' does not evaluate to a function.",
										ie.what(),
										node.get_child(grammar::fun_call_ast_node::function_index).pretty_print())
						};
					}
				}
				catch (const exception::arity_error& e)
				{
					throw exception::eval_error{
							std_format::format("arity_error '{}' with function '{}' called.", e.what(), node.get_child(grammar::fun_call_ast_node::function_index).identifier())};
				}
				catch (const exception::guard_error& e)
				{
					throw exception::eval_error{
							std_format::format("guard_error '{}' with function '{}' called.", e.what(), node.get_child(grammar::fun_call_ast_node::function_index).identifier())};
				}
				catch (interrupt_type::interrupt_return& ret) { return std::move(ret.value); }
			}

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override { return do_eval<true>(*this, state, visitor); }

		public:
			GAL_AST_SET_RTTI(fun_call_ast_node)

			fun_call_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} { gal_assert(not this->empty()); }
		};

		struct unused_return_fun_call_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override { return fun_call_ast_node::do_eval<false>(*this, state, visitor); }

		public:
			GAL_AST_SET_RTTI(unused_return_fun_call_ast_node)

			unused_return_fun_call_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} { gal_assert(not this->empty()); }
		};

		struct array_access_ast_node final : ast_node
		{
		private:
			mutable foundation::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				const foundation::scoped_function_scope scoped_function{state};

				const std::array params{
						this->get_child(grammar::array_access_ast_node::operation_target_index).eval(state, visitor),
						this->get_child(grammar::array_access_ast_node::operation_parameter_index).eval(state, visitor)};

				try
				{
					state.stack().push_params(params);
					return state->call_function(foundation::container_subscript_interface_name::value, location_, params);
				}
				catch (const exception::dispatch_error& e) { throw exception::eval_error{std_format::format("Can not find appropriate array lookup operator '{}'", foundation::container_subscript_interface_name::value), e.parameters, e.functions, false, *state}; }
			}

		public:
			GAL_AST_SET_RTTI(array_access_ast_node)

			array_access_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct dot_access_ast_node final : ast_node
		{
		private:
			foundation::string_view_type function_name_;

			mutable foundation::dispatcher::function_cache_location_type location_{};
			mutable foundation::dispatcher::function_cache_location_type array_location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				const foundation::scoped_function_scope scoped_function{state};

				auto ret{this->get_child(grammar::dot_access_ast_node::target_index).eval(state, visitor)};
				foundation::parameters_type params{ret};

				const bool has_function_params = [this, &params, &state, &visitor]
				{
					if (this->get_child(grammar::dot_access_ast_node::function_index).size() > 1)
					{
						std::ranges::for_each(
								this->get_child(grammar::dot_access_ast_node::function_index).get_child(grammar::dot_access_ast_node::function_parameter_index).view(),
								[&params, &state, &visitor](auto& c) { params.push_back(c.eval(state, visitor)); });

						return true;
					}
					return false;
				}();

				const foundation::parameters_view_type ps{params};
				state.stack().push_params(ps);
				try { ret = state->call_member_function(function_name_, location_, ps, has_function_params); }
				catch (const exception::dispatch_error& e)
				{
					if (e.functions.empty()) { throw exception::eval_error{std_format::format("'{}' is not a function", function_name_)}; }
					throw exception::eval_error{std_format::format("{} for function '{}' called", e.what(), function_name_), e.parameters, e.functions, true, *state};
				}
				catch (interrupt_type::interrupt_return& r) { ret = std::move(r.value); }

				if (auto& c = this->get_child(grammar::dot_access_ast_node::function_index);
					c.is<array_access_ast_node>())
				{
					try
					{
						const foundation::parameters_type p{
								ret,
								c.get_child(grammar::array_access_ast_node::operation_parameter_index).eval(state, visitor)
						};
						ret = state->call_function(foundation::container_subscript_interface_name::value, array_location_, p);
					}
					catch (const exception::dispatch_error& e) { throw exception::eval_error{std_format::format("Can not find appropriate array lookup operator '{}'", foundation::container_subscript_interface_name::value), e.parameters, e.functions, false, *state}; }
				}

				return ret;
			}

		public:
			GAL_AST_SET_RTTI(dot_access_ast_node)

			dot_access_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)},
				  function_name_{
						  this->get_child(grammar::dot_access_ast_node::function_index).is_any<fun_call_ast_node, array_access_ast_node>() ? this->get_child(grammar::dot_access_ast_node::function_index).get_child(grammar::dot_access_ast_node::function_secondary_index).identifier() : this->get_child(grammar::dot_access_ast_node::function_index).identifier()} {}
		};

		struct arg_ast_node final : ast_node
		{
			GAL_AST_SET_RTTI(arg_ast_node)

			arg_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct arg_list_ast_node final : ast_node
		{
			GAL_AST_SET_RTTI(arg_list_ast_node)

			arg_list_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}

			[[nodiscard]] static identifier_type get_arg_name(const ast_node& node)
			{
				if (node.empty()) { return node.identifier(); }
				// without type identity
				if (node.size() == 1) { return node.get_child(grammar::arg_ast_node::type_or_name_index).identifier(); }
				// with type identity
				return node.get_child(grammar::arg_ast_node::name_index).identifier();
			}

			[[nodiscard]] static auto get_arg_names(const ast_node& node) { return node.view() | std::views::transform([](const auto& child) { return arg_list_ast_node::get_arg_name(child); }); }

			static foundation::parameter_type_mapper::parameter_type_mapping_type::value_type get_arg_type(const ast_node& node, const foundation::dispatcher_state& state)
			{
				// without type identity, just skip it
				if (node.size() < 2) { return {}; }

				return {
						node.get_child(grammar::arg_ast_node::type_or_name_index).identifier(),
						state->get_type_info(node.get_child(grammar::arg_ast_node::type_or_name_index).identifier(), false)
				};
			}

			static foundation::parameter_type_mapper get_arg_types(const ast_node& node, const foundation::dispatcher_state& state)
			{
				const auto view = node.view() | std::views::transform([&state](const auto& child) { return get_arg_type(child, state); });

				return foundation::parameter_type_mapper{view.begin(), view.end()};
			}
		};

		struct equation_ast_node final : ast_node
		{
		private:
			foundation::algebraic_operations operation_;

			mutable foundation::dispatcher::function_cache_location_type location_{};
			mutable foundation::dispatcher::function_cache_location_type clone_location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				foundation::scoped_function_scope function_scope{state};

				auto params = [&]
				{
					// The RHS *must* be evaluated before the LHS
					// consider `var name_same_with_function = name_same_with_function(x)`
					// if we declare the variable in scope first, then the name lookup fails
					// for the RHS
					auto rhs = this->get_child(grammar::equation_ast_node::rhs_index).eval(state, visitor);
					auto lhs = this->get_child(grammar::equation_ast_node::lhs_index).eval(state, visitor);
					return foundation::parameters_type{std::move(lhs), std::move(rhs)};
				}();

				if (params[grammar::equation_ast_node::lhs_index].is_xvalue()) { throw exception::eval_error{"Error, can not assign to a temporary value"}; }
				if (params[grammar::equation_ast_node::lhs_index].is_const()) { throw exception::eval_error{"Error, can not assign to a immutable value"}; }

				if (
					operation_ != foundation::algebraic_operations::unknown &&
					params[grammar::equation_ast_node::lhs_index].type_info().is_arithmetic() &&
					params[grammar::equation_ast_node::rhs_index].type_info().is_arithmetic())
				{
					try
					{
						return types::number_type::binary_invoke(
								operation_,
								params[grammar::equation_ast_node::lhs_index],
								params[grammar::equation_ast_node::rhs_index]);
					}
					catch (const std::exception&) { throw exception::eval_error{"Error with unsupported arithmetic assignment operation"}; }
				}
				if (operation_ == foundation::algebraic_operations::assign)
				{
					try
					{
						if (params[grammar::equation_ast_node::lhs_index].is_undefined())
						{
							if (
								// the left side variable is a reference
								this->get_child(grammar::equation_ast_node::lhs_index).is<reference_ast_node>() ||
								// or the left side is a global_decl_ast_node
								not this->get_child(grammar::equation_ast_node::lhs_index).empty() &&
								this->get_child(grammar::equation_ast_node::lhs_index).get_child(grammar::global_decl_ast_node::index).is<reference_ast_node>())
							{
								// todo: this does not handle the case of an unassigned reference variable being assigned outside of its declaration
								params[grammar::equation_ast_node::lhs_index].assign(params[grammar::equation_ast_node::rhs_index]).to_lvalue();
								return params[grammar::equation_ast_node::lhs_index];
							}
							params[grammar::equation_ast_node::rhs_index] = eval_detail::clone_if_necessary(std::move(params[grammar::equation_ast_node::rhs_index]), clone_location_, state);
						}

						try { return state->call_function(this->identifier(), location_, params); }
						catch (const exception::dispatch_error& e)
						{
							throw exception::eval_error{
									std_format::format("dispatch_error, can not find appropriate '{}' operator", this->identifier()),
									e.parameters,
									e.functions,
									false,
									*state};
						}
					}
					catch (const exception::dispatch_error& e)
					{
						throw exception::eval_error{
								"dispatch_error, missing clone or copy constructor for right hand side of equation",
								e.parameters,
								e.functions,
								false,
								*state};
					}
				}
				if (this->identifier() == foundation::operator_reference_assign_name::value)
				{
					if (
						params[grammar::equation_ast_node::lhs_index].is_undefined() ||
						params[grammar::equation_ast_node::lhs_index].type_match(params[grammar::equation_ast_node::rhs_index]))
					{
						params[grammar::equation_ast_node::lhs_index].assign(params[grammar::equation_ast_node::rhs_index]).to_lvalue();
						return params[grammar::equation_ast_node::lhs_index];
					}
					throw exception::eval_error{"Assign failed, mismatched types in equation"};
				}

				try { return state->call_function(this->identifier(), location_, params); }
				catch (const exception::dispatch_error& e)
				{
					throw exception::eval_error{
							std_format::format("dispatch_error, can not find appropriate '{}' operator", this->identifier()),
							e.parameters,
							e.functions,
							false,
							*state};
				}
			}

		public:
			GAL_AST_SET_RTTI(equation_ast_node)

			equation_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)},
				  operation_{foundation::algebraic_operation(this->identifier())} { gal_assert(this->size() == 2); }
		};

		struct global_decl_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base&) override
			{
				const auto name = [&]() -> foundation::string_view_type
				{
					if (this->get_child(grammar::global_decl_ast_node::index).is<reference_ast_node>()) { return this->get_child(grammar::global_decl_ast_node::index).get_child(grammar::global_decl_ast_node::secondary_index).identifier(); }
					return this->get_child(grammar::global_decl_ast_node::index).identifier();
				}();

				return state->add_global_mutable_no_throw(name, {});
			}

		public:
			GAL_AST_SET_RTTI(global_decl_ast_node)

			global_decl_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct var_decl_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base&) override
			{
				const auto& name = this->get_child(grammar::var_decl_ast_node::index).identifier();

				try { return state->add_local_or_throw(name, foundation::boxed_value{}); }
				catch (const exception::name_conflict_error& e) { throw exception::eval_error{std_format::format("Variable redefined '{}'", e.which())}; }
			}

		public:
			GAL_AST_SET_RTTI(var_decl_ast_node)

			var_decl_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct assign_decl_ast_node final : ast_node
		{
		private:
			mutable foundation::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				const auto& name = this->get_child(grammar::assign_decl_ast_node::lhs_index).identifier();

				try
				{
					auto object = eval_detail::clone_if_necessary(this->get_child(grammar::assign_decl_ast_node::rhs_index).eval(state, visitor), location_, state);
					object.to_lvalue();
					state->add_local_or_throw(name, object);
					return object;
				}
				catch (const exception::name_conflict_error& e) { throw exception::eval_error{std_format::format("Variable redefined '{}'", e.which())}; }
			}

		public:
			GAL_AST_SET_RTTI(assign_decl_ast_node)

			assign_decl_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct class_decl_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				foundation::scoped_scope scoped_scope{state};

				// todo: do this better
				// todo: name?
				// put class name in current scope, so it can be looked up by the members and methods
				state->add_local_or_throw("_current_class_name", const_var(this->get_child(grammar::class_decl_ast_node::name_index).identifier()));

				// register class member variables and member functions
				this->get_child(grammar::class_decl_ast_node::body_index).eval(state, visitor);

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(class_decl_ast_node)

			class_decl_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct member_decl_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base&) override
			{
				const auto& class_name = this->get_child(grammar::member_decl_ast_node::class_name_index).identifier();

				try
				{
					const auto& member_name = this->get_child(grammar::member_decl_ast_node::member_name_index).identifier();

					state->add_function(member_name,
					                    std::make_shared<foundation::dynamic_function>(
							                    class_name,
							                    fun([member_name](const foundation::dynamic_object& object) { return object.get_attr(member_name); }),
							                    true));
				}
				catch (const exception::name_conflict_error& e) { throw exception::eval_error{std_format::format("Member redefined '{}'", e.which())}; }

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(member_decl_ast_node)

			member_decl_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct def_ast_node final : ast_node
		{
			using shared_node_type = std::shared_ptr<ast_node>;

			shared_node_type body_node;
			shared_node_type guard_node;

			[[nodiscard]] static shared_node_type get_body_node(children_type&& children)
			{
				// is not accessed from index, because we know body_node is always at the end
				return std::move(children.back());
			}


			[[nodiscard]] static bool has_guard_node(const children_type& children) noexcept
			{
				// name + body => pass
				// name + arg_list + body / name + guard + body
				if (children.size() == grammar::def_ast_node::body_index) { return not children[grammar::def_ast_node::arg_list_or_guard_or_body_index]->is<arg_list_ast_node>(); }
				// name + arg_list + guard + body
				if (children.size() > grammar::def_ast_node::body_index)
				{
					gal_assert(children[grammar::def_ast_node::arg_list_or_guard_or_body_index]->is<arg_list_ast_node>());
					return true;
				}

				return false;
			}

			[[nodiscard]] static shared_node_type get_guard_node(children_type&& children, const bool has_guard)
			{
				// name [arg_list] [guard] body
				if (has_guard) { return std::move(*(children.end() - 2)); }
				return {};
			}

			[[nodiscard]] static auto get_arg_list_info(const ast_node& node, const children_type::difference_type index, const foundation::dispatcher_state& state)
			{
				struct param_pack
				{
					decltype(std::declval<const ast_node&>().size()) num_params;
					decltype(arg_list_ast_node::get_arg_names(std::declval<const ast_node&>())) param_names;
					decltype(arg_list_ast_node::get_arg_types(std::declval<const ast_node&>(), std::declval<const foundation::dispatcher_state&>())) param_types;
				};

				if (static_cast<children_type::difference_type>(node.size()) > index && node.get_child(index).is<arg_list_ast_node>())
				{
					return param_pack{
							.num_params = node.get_child(index).size(),
							.param_names = arg_list_ast_node::get_arg_names(node.get_child(index)),
							.param_types = arg_list_ast_node::get_arg_types(node.get_child(index), state)};
				}

				return param_pack{
						.num_params = 0,
						// todo: cannot default construct?
						// .param_names = {},
						// todo: fix it!!!
						.param_names = arg_list_ast_node::get_arg_names(node),
						.param_types = {}};
			}

		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				auto [num_params, param_names, param_types] = get_arg_list_info(*this, grammar::def_ast_node::arg_list_or_guard_or_body_index, state);

				auto dispatcher = std::ref(*state);

				auto guard = [this, dispatcher, num_params, param_names, &visitor]
				{
					if (guard_node)
					{
						return foundation::make_dynamic_function_proxy(
								[this, dispatcher, param_names, &visitor](const foundation::parameters_view_type params)
								{
									return eval_detail::eval_function(
											dispatcher,
											*guard_node,
											visitor,
											params,
											param_names);
								},
								static_cast<foundation::function_proxy_base::arity_size_type>(num_params),
								guard_node);
					}
					return foundation::function_proxy_type{};
				}();

				try
				{
					const auto& name = this->get_child(grammar::def_ast_node::function_name_index).identifier();
					state->add_function(
							name,
							make_dynamic_function_proxy(
									[this, dispatcher, param_names, &visitor](const foundation::parameters_view_type params)
									{
										return eval_detail::eval_function(
												dispatcher,
												*body_node,
												visitor,
												params,
												param_names);
									},
									static_cast<foundation::function_proxy_base::arity_size_type>(num_params),
									body_node,
									std::move(param_types),
									std::move(guard)));
				}
				catch (const exception::name_conflict_error& e) { throw exception::eval_error{std_format::format("Function redefined '{}'", e.which())}; }

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(def_ast_node)

			def_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{
						  get_rtti_index(),
						  identifier,
						  location,
						  children_type{
								  std::make_move_iterator(children.begin()),
								  // arg_list may also not exist, so we can only guarantee to keep the body and optional guard
								  std::make_move_iterator(children.end() - (has_guard_node(children) ? 2 : 1))}},
				  // This apparent use after move is safe because we are only
				  // moving out the specific elements we need on each operation.
				  body_node{get_body_node(std::move(children))},
				  // children.size() - this->size() == 2 => guard + body
				  guard_node{get_guard_node(std::move(children), children.size() - this->size() == 2)} { }
		};

		struct method_ast_node final : ast_node
		{
			using shared_node_type = std::shared_ptr<ast_node>;

			shared_node_type body_node;
			shared_node_type guard_node;

		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				// The first param of a method is always the implied this ptr.
				std::vector<std::string_view> param_names{foundation::object_self_name::value};

				auto param_types = [&param_names, &state, this]
				{
					if (const auto& node = this->get_child(grammar::method_ast_node::arg_list_index);
						// the arg_list is optional
						node.is<arg_list_ast_node>())
					{
						auto args = arg_list_ast_node::get_arg_names(node);
						param_names.reserve(param_names.size() + args.size());
						param_names.insert(param_names.end(), args.begin(), args.end());
						return arg_list_ast_node::get_arg_types(node, state);
					}
					return decltype(arg_list_ast_node::get_arg_types(*this, state)){};
				}();

				const auto num_params = param_names.size();

				auto dispatcher = std::ref(*state);

				auto guard = [this, dispatcher, num_params, &param_names, &visitor]
				{
					if (guard_node)
					{
						return foundation::make_dynamic_function_proxy(
								[this, dispatcher, &param_names, &visitor](const foundation::parameters_view_type params) { return eval_detail::eval_function(dispatcher, *guard_node, visitor, params, param_names); },
								static_cast<foundation::function_proxy_base::arity_size_type>(num_params),
								guard_node);
					}
					return foundation::function_proxy_type{};
				}();

				const auto& class_name = this->get_child(grammar::method_ast_node::class_name_index).identifier();
				const auto& function_name = this->get_child(grammar::method_ast_node::function_name_index).identifier();

				try
				{
					if (function_name == class_name)
					{
						// constructor
						param_types.add(class_name, {});

						state->add_function(
								function_name,
								std::make_shared<foundation::dynamic_constructor>(
										class_name,
										make_dynamic_function_proxy(
												[this, dispatcher, &param_names, &visitor](const foundation::parameters_view_type params) { return eval_detail::eval_function(dispatcher, *body_node, visitor, params, param_names); },
												static_cast<foundation::function_proxy_base::arity_size_type>(num_params),
												body_node,
												std::move(param_types),
												std::move(guard))));
					}
					else
					{
						// if the type is unknown, then this generates a function that looks up the type
						// at runtime. Defining the type first before this is called is better
						param_types.add(class_name, state->get_type_info(class_name, false));

						state->add_function(
								function_name,
								std::make_shared<foundation::dynamic_function>(
										class_name,
										make_dynamic_function_proxy(
												[this, dispatcher, &param_names, &visitor](const foundation::parameters_view_type params) { return eval_detail::eval_function(dispatcher, *body_node, visitor, params, param_names); },
												static_cast<foundation::function_proxy_base::arity_size_type>(num_params),
												body_node,
												std::move(param_types),
												std::move(guard))));
					}
				}
				catch (const exception::name_conflict_error& e) { throw exception::eval_error{std_format::format("Method redefined '{}'", e.which())}; }

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(method_ast_node)

			method_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{
						  get_rtti_index(),
						  identifier,
						  location,
						  children_type{
								  std::make_move_iterator(children.begin()),
								  // arg_list may also not exist, so we can only guarantee to keep the body and optional guard
								  std::make_move_iterator(children.end() - (def_ast_node::has_guard_node(children) ? 2 : 1))}},
				  // This apparent use after move is safe because we are only
				  // moving out the specific elements we need on each operation.
				  body_node{def_ast_node::get_body_node(std::move(children))},
				  // children.size() - this->size() == 2 => guard + body
				  guard_node{def_ast_node::get_guard_node(std::move(children), children.size() - this->size() == 2)} { }
		};

		struct lambda_ast_node final : ast_node
		{
			using shared_node_type = std::shared_ptr<ast_node>;

		private:
			shared_node_type lambda_node_;

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				const auto& [captures, is_capture_this] = [&]
				{
					foundation::engine_stack::scope_type named_captures;
					bool capture_this = false;

					std::ranges::for_each(
							this->get_child(grammar::lambda_ast_node::capture_list_index).view(),
							[&named_captures, &capture_this, &state, &visitor](auto& c)
							{
								// the lambda's list of captured variables does not allow specifying a type (the type is obtained from the captured variable)
								auto& capture = c.get_child(grammar::arg_ast_node::type_or_name_index);
								// changed since 0.5.4, see engine_stack::scope_type
								// named_captures.emplace(cf.identifier(), cf.eval(state, visitor));
								named_captures.emplace_back(capture.identifier(), capture.eval(state, visitor));
								capture_this = capture.identifier() == foundation::object_self_name::value;
							});

					return std::make_pair(std::move(named_captures), capture_this);
				}();

				auto [num_params, param_names, param_types] = def_ast_node::get_arg_list_info(*this, grammar::lambda_ast_node::function_parameter_or_body_index, state);

				auto dispatcher = std::ref(*state);

				return foundation::boxed_value{
						make_dynamic_function_proxy(
								[dispatcher, this, &captures, is_capture_this, param_names, &visitor](const foundation::parameters_view_type params) { return eval_detail::eval_function(dispatcher, *lambda_node_, visitor, params, param_names, captures, is_capture_this); },
								static_cast<foundation::function_proxy_base::arity_size_type>(num_params),
								lambda_node_,
								std::move(param_types))};
			}

		public:
			GAL_AST_SET_RTTI(lambda_ast_node)

			lambda_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{
						  get_rtti_index(),
						  identifier,
						  location,
						  children_type{
								  std::make_move_iterator(children.begin()),
								  // parameter arg_list may also not exist, so we can only guarantee to keep the body
								  std::make_move_iterator(children.end() - 1)}},
				  // This apparent use after move is safe because we are only
				  // moving out the specific elements we need on each operation.
				  lambda_node_{std::move(children.back())} { }
		};

		struct block_ast_node;

		struct no_scope_block_ast_node final : ast_node
		{
			friend struct block_ast_node;

		private:
			static foundation::boxed_value do_eval(ast_node& node, const foundation::dispatcher_state& state, ast_visitor_base& visitor)
			{
				std::ranges::for_each(
						node.front_view(static_cast<children_type::difference_type>(node.size()) - 1),
						[&state, &visitor](auto& c) { c.eval(state, visitor); });

				return node.back().eval(state, visitor);
			}

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override { return do_eval(*this, state, visitor); }

		public:
			GAL_AST_SET_RTTI(no_scope_block_ast_node)

			no_scope_block_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct block_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				foundation::scoped_scope scoped_scope{state};
				return no_scope_block_ast_node::do_eval(*this, state, visitor);
			}

		public:
			GAL_AST_SET_RTTI(block_ast_node)

			block_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct if_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				if (get_bool_condition(this->get_child(grammar::if_ast_node::condition_index).eval(state, visitor), state)) { return this->get_child(grammar::if_ast_node::true_branch_index).eval(state, visitor); }
				return this->get_child(grammar::if_ast_node::false_branch_index).eval(state, visitor);
			}

		public:
			GAL_AST_SET_RTTI(if_ast_node)

			if_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} { gal_assert(this->size() == 3); }
		};

		struct while_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				foundation::scoped_scope scoped_scope{state};

				try
				{
					while (get_scoped_bool_condition(this->get_child(grammar::while_ast_node::condition_index), state, visitor))
					{
						try { this->get_child(grammar::while_ast_node::body_index).eval(state, visitor); }
						catch (const interrupt_type::interrupt_continue&)
						{
							// we got a continued exception, which means all the remaining
							// loop implementation is skipped, and we just need to continue to
							// the next condition test
						}
					}
				}
				catch (const interrupt_type::interrupt_break&)
				{
					// loop was broken intentionally
				}

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(while_ast_node)

			while_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct ranged_for_ast_node final : ast_node
		{
		private:
			using location_type = foundation::dispatcher::function_cache_location_type;

			mutable location_type view_location_{};
			mutable location_type empty_location_{};
			mutable location_type star_location_{};
			mutable location_type advance_location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				// todo
				const auto& loop_var_name = this->get_child(grammar::ranged_for_ast_node::loop_variable_name_index).get_child(0).identifier();
				const auto range_expression_result = this->get_child(grammar::ranged_for_ast_node::loop_range_name_index).eval(state, visitor);

				// range_type
				if (range_expression_result.type_info().bare_equal(typeid(types::range_type)))
				{
					auto& range = boxed_cast<types::range_type&>(range_expression_result);

					do
					{
						foundation::scoped_scope scoped_scope{state};
						state->add_local_or_throw(loop_var_name, foundation::boxed_value{range.get()});

						try { this->get_child(grammar::ranged_for_ast_node::body_index).eval(state, visitor); }
						catch (const interrupt_type::interrupt_continue&) { }
						catch (const interrupt_type::interrupt_break&)
						{
							// loop broken
							break;
						}
					} while (range.next());

					return void_var();
				}

				// other container type
				const auto get_function = [&state](const foundation::string_view_type name, location_type& location)
				{
					if (location.has_value()) { return *location; }

					auto func = state->get_function(name);
					location.emplace(func);
					return func;
				};

				const auto call_function = [&state](const auto& function, const auto& param) { return foundation::dispatch(*function, foundation::parameters_view_type{param}, state.convertor_state()); };

				const auto view_function = get_function(foundation::container_view_interface_name::value, view_location_);
				const auto empty_function = get_function(foundation::container_view_empty_interface_name::value, empty_location_);
				const auto star_function = get_function(foundation::container_view_star_interface_name::value, star_location_);
				const auto advance_function = get_function(foundation::container_view_advance_interface_name::value, advance_location_);

				for (
					// get the view
					const auto view = call_function(view_function, range_expression_result);
					// while view not empty
					not boxed_cast<bool>(call_function(empty_function, view));
					// advance the iterator
					(void)call_function(advance_function, view))
				{
					foundation::scoped_scope scoped_scope{state};
					// push the value into the stack
					state->add_local_or_throw(loop_var_name, call_function(star_function, view));

					try { this->get_child(grammar::ranged_for_ast_node::body_index).eval(state, visitor); }
					catch (const interrupt_type::interrupt_continue&) { }
					catch (const interrupt_type::interrupt_break&)
					{
						// loop broken
						break;
					}
				}

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(ranged_for_ast_node)

			ranged_for_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} { gal_assert(this->size() == 3); }
		};

		struct break_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state&, ast_visitor_base&) override
			{
				// todo: better way
				throw interrupt_type::interrupt_break{};
			}

		public:
			GAL_AST_SET_RTTI(break_ast_node)

			break_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct continue_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state&, ast_visitor_base&) override
			{
				// todo: better way
				throw interrupt_type::interrupt_continue{};
			}

		public:
			GAL_AST_SET_RTTI(continue_ast_node)

			continue_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct return_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				// todo: better way
				if (not this->empty()) { throw interrupt_type::interrupt_return{this->get_child(grammar::return_ast_node::operation_index).eval(state, visitor)}; }
				throw interrupt_type::interrupt_return{void_var()};
			}

		public:
			GAL_AST_SET_RTTI(return_ast_node)

			return_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct file_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				try
				{
					if (const auto size = this->size(); size > 0)
					{
						std::ranges::for_each(
								this->front_view(static_cast<children_type::difference_type>(size) - 1),
								[&state, &visitor](auto& child) { child.eval(state, visitor); });
						return this->back().eval(state, visitor);
					}

					return void_var();
				}
				catch (const interrupt_type::interrupt_continue&) { throw exception::eval_error{"Unexpected 'continue' statement outside of a loop"}; }
				catch (const interrupt_type::interrupt_break&) { throw exception::eval_error{"Unexpected 'break' statement outside of a loop"}; }
			}

		public:
			GAL_AST_SET_RTTI(file_ast_node)

			file_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct match_default_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				foundation::scoped_scope scoped_scope{state};

				this->get_child(grammar::match_default_ast_node::body_index).eval(state, visitor);

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(match_default_ast_node)

			match_default_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} { gal_assert(this->size() == 1); }
		};

		struct match_case_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				foundation::scoped_scope scoped_scope{state};

				this->get_child(grammar::match_case_ast_node::body_index).eval(state, visitor);

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(match_case_ast_node)

			match_case_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} { gal_assert(this->size() == 2); }
		};

		struct match_fallthrough_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state&, ast_visitor_base&) override
			{
				// todo: better way
				throw interrupt_type::interrupt_continue{};
			}

		public:
			GAL_AST_SET_RTTI(match_fallthrough_ast_node)

			match_fallthrough_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} { gal_assert(this->empty()); }
		};

		struct match_ast_node final : ast_node
		{
		private:
			mutable foundation::dispatcher::function_cache_location_type location_;

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				foundation::scoped_scope scoped_scope{state};

				std::array<foundation::boxed_value, 2> match_value{this->get_child(grammar::match_ast_node::match_value_index).eval(state, visitor)};

				bool breaking = false;
				children_type::difference_type current_case = 0;
				bool has_matched = false;
				// from 1 to size
				while (not breaking && ++current_case < static_cast<children_type::difference_type>(this->size()))
				{
					if (auto& current = this->get_child(current_case); current.is<match_case_ast_node>())
					{
						try
						{
							if (
								// fallthrough
								has_matched ||
								// new match
								[&state, &visitor, this, &match_value, &current]
								{
									match_value[1] = current.get_child(grammar::match_case_ast_node::match_value_index).eval(state, visitor);
									return boxed_cast<bool>(
											state->call_function(
													foundation::operator_equal_name::value,
													location_,
													match_value));
								}())
							{
								has_matched = true;

								try { (void)current.eval(state, visitor); }
								catch (const interrupt_type::interrupt_continue&)
								{
									// fallthrough
								}
								catch (const interrupt_type::interrupt_break&)
								{
									// break
									breaking = true;
								}
							}
						}
						catch (const exception::bad_boxed_cast&) { throw exception::eval_error{"Internal error: case guard evaluation not boolean"}; }
					}
					else if (current.is<match_default_ast_node>())
					{
						has_matched = true;
						current.eval(state, visitor);
						breaking = true;
					}
				}

				return void_var();
			}

		public:
			GAL_AST_SET_RTTI(match_ast_node)

			match_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct logical_and_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				return const_var(
						get_bool_condition(this->get_child(grammar::logical_and_ast_node::lhs_index).eval(state, visitor), state) &&
						get_bool_condition(this->get_child(grammar::logical_and_ast_node::rhs_index).eval(state, visitor), state));
			}

		public:
			GAL_AST_SET_RTTI(logical_and_ast_node)

			logical_and_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} { gal_assert(this->size() == 2); }
		};

		struct logical_or_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				return const_var(
						get_bool_condition(this->get_child(grammar::logical_or_ast_node::lhs_index).eval(state, visitor), state) ||
						get_bool_condition(this->get_child(grammar::logical_or_ast_node::rhs_index).eval(state, visitor), state));
			}

		public:
			GAL_AST_SET_RTTI(logical_or_ast_node)

			logical_or_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} { gal_assert(this->size() == 2); }
		};

		struct inline_list_ast_node final : ast_node
		{
		private:
			mutable foundation::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				return const_var(
						[this, &state, &visitor]
						{
							try
							{
								types::list_type result{};

								if (not this->empty())
								{
									auto& args = this->get_child(grammar::inline_list_ast_node::arg_list_index);
									// result.reserve(cs.size());
									std::ranges::for_each(
											args.view(),
											[this, &result, &state, &visitor](auto& arg) { result.push_back(eval_detail::clone_if_necessary(arg.eval(state, visitor), location_, state)); });
								}

								return result;
							}
							catch (const exception::dispatch_error& e)
							{
								throw exception::eval_error{
										std_format::format("Can not find appropriate '{}' or copy constructor while insert elements into list", foundation::object_clone_interface_name::value),
										e.parameters,
										e.functions,
										false,
										*state};
							}
						}());
			}

		public:
			GAL_AST_SET_RTTI(inline_list_ast_node)

			inline_list_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct map_pair_ast_node final : ast_node
		{
			GAL_AST_SET_RTTI(map_pair_ast_node)

			map_pair_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct inline_map_ast_node final : ast_node
		{
		private:
			mutable foundation::dispatcher::function_cache_location_type location_{};

			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				return const_var(
						[this, &state, &visitor]
						{
							try
							{
								types::dict_type result{};

								std::ranges::for_each(
										this->get_child(grammar::inline_map_ast_node::arg_list_index).view(),
										[this, &result, &state, &visitor](auto& arg)
										{
											result.emplace(
													// note: see standard_library.hpp -> dict_type
													state->boxed_cast<types::dict_type::key_type>(arg.get_child(grammar::map_pair_ast_node::key_index).eval(state, visitor)),
													eval_detail::clone_if_necessary(arg.get_child(grammar::map_pair_ast_node::value_index).eval(state, visitor), location_, state));
										});

								return result;
							}
							catch (const exception::dispatch_error& e)
							{
								throw exception::eval_error{
										std_format::format("Can not find appropriate '{}' or copy constructor while insert elements into map", foundation::object_clone_interface_name::value),
										e.parameters,
										e.functions,
										false,
										*state};
							}
						}());
			}

		public:
			GAL_AST_SET_RTTI(inline_map_ast_node)

			inline_map_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct try_catch_ast_node final : ast_node
		{
			GAL_AST_SET_RTTI(try_catch_ast_node)

			try_catch_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct try_finally_ast_node final : ast_node
		{
			GAL_AST_SET_RTTI(try_finally_ast_node)

			try_finally_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		struct try_ast_node final : ast_node
		{
		private:
			[[nodiscard]] foundation::boxed_value do_eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor) override
			{
				auto finalize = [this, &state, &visitor]
				{
					if (auto& back = this->back();
						back.is<try_finally_ast_node>()) { back.get_child(grammar::try_finally_ast_node::body_index).eval(state, visitor); }
				};

				auto handle_and_finalize = [this, &state, &visitor, finalize]<typename E>(const E& exception)
							requires(std::is_base_of_v<std::exception, E> || std::is_same_v<E, foundation::boxed_value>)
				{
					auto ret = [this, &state, &visitor, finalize](const E& e)
					{
						auto end_point = this->size();
						if (this->back().is<try_finally_ast_node>())
						{
							gal_assert(end_point > 0);
							end_point = this->size() - 1;
						}

						return [this, &state, &visitor, finalize, end_point, exception = [](const E& exc)
						{
							if constexpr (std::is_same_v<E, foundation::boxed_value>) { return exc; }
							else { return foundation::boxed_value{std::ref(exc)}; }
						}(e)]
						{
							for (decltype(this->size()) i = 1; i < end_point; ++i)
							{
								foundation::scoped_scope scoped_scope{state};

								auto& catch_block = this->get_child(static_cast<children_type::difference_type>(i));

								// no variable capture
								if (catch_block.size() == 1) { return catch_block.get_child(grammar::try_catch_ast_node::argument_or_body_index).eval(state, visitor); }

								if (catch_block.size() == 2 || catch_block.size() == 3)
								{
									const auto& name = arg_list_ast_node::get_arg_name(catch_block.get_child(grammar::try_catch_ast_node::argument_or_body_index));

									if (foundation::parameter_type_mapper{{arg_list_ast_node::get_arg_type(catch_block.get_child(grammar::try_catch_ast_node::argument_or_body_index), state)}}.match(foundation::parameters_view_type{exception}, state.convertor_state()).first)
									{
										state->add_local_or_throw(name, exception);

										// variable capture
										if (catch_block.size() == 2) { return catch_block.get_child(grammar::try_catch_ast_node::body_index).eval(state, visitor); }
									}
								}
								else
								{
									finalize();
									throw exception::eval_error{"Internal error: catch block size unrecognized"};
								}
							}

							return foundation::boxed_value{};
						}();
					}(exception);

					finalize();

					return ret;
				};

				foundation::scoped_scope scoped_scope{state};

				try { return this->get_child(grammar::try_ast_node::body_index).eval(state, visitor); }
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
			GAL_AST_SET_RTTI(try_ast_node)

			try_ast_node(
					const identifier_type identifier,
					const parse_location location,
					children_type&& children)
				: ast_node{get_rtti_index(), identifier, location, std::move(children)} {}
		};

		inline foundation::boxed_value ast_node::eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor)
		{
			visitor.visit(*this);

			// todo

			try { return do_eval(state, visitor); }
			catch (exception::eval_error& e)
			{
				e.stack_traces.emplace_back(*this);
				throw;
			}
		}
	}

	namespace exception
	{
		inline void eval_error::pretty_print_to(string_type& dest) const
		{
			dest.append(what());
			if (not stack_traces.empty())
			{
				std_format::format_to(
						std::back_inserter(dest),
						"during evaluation at file '{}'({}).\n\n{}\n\t{}",
						stack_traces.front().filename(),
						stack_traces.front().pretty_position_print(),
						detail,
						stack_traces.front().pretty_print());

				std::ranges::for_each(
						stack_traces | std::views::drop(1),
						[&dest](const auto& trace)
						{
							if (not trace.template is_any<ast::block_ast_node, ast::file_ast_node>())
							{
								std_format::format_to(
										std::back_inserter(dest),
										"\n\tfrom file '{}'({}).\n\t{}.",
										trace.filename(),
										trace.pretty_position_print(),
										trace.pretty_print());
							}
						});
			}

			dest.push_back('\n');
		}
	}
}

#endif // GAL_LANG_LANGUAGE_EVAL_HPP
