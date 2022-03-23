#pragma once

#ifndef GAL_LANG_FOUNDATION_PROXY_FUNCTION_HPP
#define GAL_LANG_FOUNDATION_PROXY_FUNCTION_HPP

#include <gal/boxed_cast.hpp>
#include <gal/foundation/dynamic_object.hpp>
#include <gal/foundation/parameters.hpp>
#include <gal/foundation/return_handler.hpp>
#include <gal/language/name.hpp>
#include <memory>
#include <ranges>

namespace gal::lang
{
	namespace lang
	{
		struct ast_node_base;
	}// namespace lang

	namespace exception
	{
		/**
			 * @brief Exception thrown when there is a mismatch in number of
			 * parameters during proxy_function execution.
			 */
		class arity_error final : public std::range_error
		{
		public:
			// it requires to be signed type
			using size_type = int;

			size_type expected;
			size_type got;

			arity_error(const size_type expected, const size_type got)
				: std::range_error{"Function dispatch arity mismatch"},
				  expected{expected},
				  got{got} {}
		};

		/**
			 * @brief Exception thrown if a function's guard fails.
			 */
		class guard_error final : public std::runtime_error
		{
		public:
			guard_error()
				: std::runtime_error{"Guard evaluation failed"} {}
		};

		class dispatch_error final : public std::runtime_error
		{
		public:
			foundation::parameters_type parameters;
			foundation::proxy_functions_type functions;

			dispatch_error(
					foundation::parameters_type&& parameters,
					foundation::immutable_proxy_functions_type&& functions,
					const std::string_view message = "Error with function dispatch")
				: std::runtime_error{message.data()},
				  parameters{std::move(parameters)},
				  functions{std::move(functions)} {}
		};
	}// namespace exception

	template<typename FunctionSignature>
	std::function<FunctionSignature> make_functor(
			foundation::immutable_proxy_function&& function,
			const foundation::type_conversion_state* conversion);

	namespace foundation
	{
		namespace proxy_function_detail
		{
			using namespace return_handler_detail;

			/**
			 * @brief Used by proxy_function_impl to return a list of all param types it contains.
			 */
			template<typename Result, typename... Params>
			type_infos_type build_params_type_list(Result (*)(Params ...)) { return {make_type_info<Result>(), make_type_info<Params>()...}; }

			/**
			 * @brief Used by proxy_function_impl to determine if it is equivalent to another
			 * proxy_function_impl object. This function is primarily used to prevent registration
			 * of two functions with the exact same signatures.
			 */
			template<typename Result, typename... Params>
			bool is_invokable(
					Result (*)(Params ...),
					const parameters_view_type params,
					const type_conversion_state& conversion) noexcept
			{
				try
				{
					[&, i = parameters_type::size_type{0}]() mutable { ((void)boxed_cast<Params>(params[i++], &conversion), ...); }();

					return true;
				}
				catch (const exception::bad_boxed_cast&) { return false; }
			}

			/**
			 * @brief Used by proxy_function_impl to perform type safe execution of a function.
			 * @note The function attempts to unbox each parameter to the expected type.
			 * if any unboxing fails the execution of the function fails and
			 * the bad_boxed_cast is passed up to the caller.
			 */
			template<typename Callable, typename Result, typename... Params>
			boxed_value do_invoke(
					Result (*)(Params ...),
					const Callable& function,
					const parameters_view_type params,
					const type_conversion_state& conversion)
			{
				auto call = [&]<bool HasReturn, std::size_t... Index>(std::index_sequence<Index...>)-> std::conditional_t<HasReturn, Result, void>
				{
					if constexpr (HasReturn) { return function(boxed_cast<Params>(params[Index], &conversion)...); }
					else { return function(boxed_cast<Params>(params[Index], &conversion)...); }
				};

				if constexpr (std::is_same_v<Result, void>)
				{
					call.decltype(call)::template operator()<false>(std::index_sequence_for<Params...>{});
					return return_handler<void>::handle();
				}
				else { return return_handler<Result>::handle(call.decltype(call)::template operator()<true>(std::index_sequence_for<Params...>{})); }
			}
		}// namespace proxy_function_detail

		class parameter_type_mapper
		{
		public:
			// todo: do not store the name, the storage of the name should be handed over to the string pool
			using parameter_name_type = string_type;
			using parameter_name_view_type = string_view_type;
			using parameter_type_type = gal_type_info;

			using parameter_type_mapping_type = std::vector<std::pair<parameter_name_type, parameter_type_type>>;

		private:
			parameter_type_mapping_type mapping_;
			bool empty_;

			bool check_empty()
			{
				return std::ranges::all_of(
						mapping_ | std::views::keys,
						[](const auto name) { return name.empty(); });
			}

			void check_empty(const parameter_name_type name) { if (empty_ && not name.empty()) { empty_ = false; } }

		public:
			parameter_type_mapper()
				: empty_{true} {}

			explicit parameter_type_mapper(parameter_type_mapping_type&& mapping)
				: mapping_{std::move(mapping)},
				  empty_{check_empty()} {}

			[[nodiscard]] bool operator==(const parameter_type_mapper& other) const { return mapping_ == other.mapping_; }

			void add(parameter_name_type name, parameter_type_type type) { check_empty(mapping_.emplace_back(name, type).first); }

			[[nodiscard]] parameters_type convert(const parameters_view_type params, const type_conversion_state& conversion) const
			{
				auto ret = params.to<std::vector>();

				const auto dynamic_object_type_info = make_type_info<dynamic_object>();

				for (decltype(ret.size()) i = 0; i < ret.size(); ++i)
				{
					// the newest is last, so from back to front
					if (const auto& [name, type] = mapping_[mapping_.size() - 1 - i];
						not name.empty())
					{
						if (const auto& object = ret[i];
							not object.type_info().bare_equal(dynamic_object_type_info))
						{
							if (not type.is_undefined())
							{
								if (not object.type_info().bare_equal(type))
								{
									if (conversion->is_convertible_type(type, object.type_info()))
									{
										try
										{
											// We will not catch any bad_boxed_dynamic_cast that is thrown, let the user get it
											// either way, we are not responsible if it doesn't work
											ret[i] = conversion->boxed_type_conversion(type, conversion.saves(), ret[i]);
										}
										catch (...)
										{
											try
											{
												// try going the other way
												ret[i] = conversion->boxed_type_down_conversion(type, conversion.saves(), ret[i]);
											}
											catch (const std::bad_any_cast&) { throw exception::bad_boxed_cast{object.type_info(), type.bare_type_info()}; }
										}
									}
								}
							}
						}
					}
				}

				return ret;
			}

			/**
		 * @return pair.first means 'is match or not', pair.second means 'needs conversions'
		 */
			[[nodiscard]] std::pair<bool, bool> match(const parameters_view_type params, const type_conversion_state& conversion) const noexcept
			{
				const auto dynamic_object_type_info = make_type_info<dynamic_object>();

				bool need_conversion = false;

				if (empty_) { return {true, need_conversion}; }
				if (params.size() != mapping_.size()) { return {false, need_conversion}; }

				for (decltype(params.size()) i = 0; i < params.size(); ++i)
				{
					// the newest is last, so from back to front
					if (const auto& [name, ti] = mapping_[mapping_.size() - 1 - i];
						not name.empty())
					{
						if (const auto& object = params[i];
							object.type_info().bare_equal(dynamic_object_type_info))
						{
							try
							{
								if (const auto& result = boxed_cast<const dynamic_object&>(object, &conversion);
									not(lang::dynamic_object_type_name::match(name) || result.type_name() == name)) { return {false, false}; }
							}
							catch (const std::bad_cast&) { return {false, false}; }
						}
						else
						{
							if (not ti.is_undefined())
							{
								if (not object.type_info().bare_equal(ti))
								{
									if (not conversion->is_convertible_type(ti, object.type_info())) { return {false, false}; }
									need_conversion = true;
								}
							}
							else { return {false, false}; }
						}
					}
				}

				return {true, need_conversion};
			}

			[[nodiscard]] const parameter_type_mapping_type& get_mapping() const noexcept { return mapping_; }
		};

		/**
		 * @brief Pure virtual base class for all proxy_function implementations.
		 * proxy_functions are a type erasure of type safe C++ function calls.
		 * At runtime parameter types are expected to be tested against passed in types.
		 * dispatch_engine only knows how to work with proxy_function, no other
		 * function classes.
		 */
		class proxy_function_base
		{
		public:
			proxy_function_base() = default;

			proxy_function_base(const proxy_function_base&) = default;
			proxy_function_base& operator=(const proxy_function_base&) = default;
			proxy_function_base(proxy_function_base&&) = default;
			proxy_function_base& operator=(proxy_function_base&&) = default;

			virtual ~proxy_function_base() noexcept = default;

			static_assert(std::is_signed_v<exception::arity_error::size_type>);
			using arity_size_type = exception::arity_error::size_type;
			constexpr static arity_size_type no_parameters_arity = -1;

		protected:
			type_infos_type types_;
			arity_size_type arity_{};
			bool has_arithmetic_param_{};

			static bool is_convertible_types(const type_infos_view_type types, const parameters_view_type params, const type_conversion_state& conversion) noexcept
			{
				if (types.size() - 1 != params.size()) { return false; }

				// todo: zip!!!
				for (decltype(params.size()) i = 0; i < params.size(); ++i) { if (not is_convertible_type(types[i + 1], params[i], conversion)) { return false; } }

				return true;
			}

			proxy_function_base(type_infos_type&& types, const arity_size_type arity)
				: types_{std::move(types)},
				  arity_{arity},
				  has_arithmetic_param_{std::ranges::any_of(types_, [](const auto& type) { return type.is_arithmetic(); })} {}

			proxy_function_base(const type_infos_type& types, const arity_size_type arity)
				: types_{types},
				  arity_{arity},
				  has_arithmetic_param_{std::ranges::any_of(types_, [](const auto& type) { return type.is_arithmetic(); })} {}

			[[nodiscard]] virtual boxed_value do_invoke(parameters_view_type params, const type_conversion_state& conversion) const = 0;

		public:
			[[nodiscard]] static bool is_convertible_type(const gal_type_info& type, const boxed_value& object, const type_conversion_state& conversion) noexcept
			{
				const auto boxed_value_type_info = make_type_info<boxed_value>();
				const auto boxed_number_type_info = make_type_info<boxed_number>();
				// ReSharper disable once CppTooWideScopeInitStatement
				const auto function_type_info = make_type_info<immutable_proxy_functions_type::value_type>();

				if (
					type.is_undefined() ||
					type.bare_equal(boxed_value_type_info) ||
					(not object.type_info().is_undefined() &&
					 ((type.bare_equal(boxed_number_type_info) && object.type_info().is_arithmetic()) ||
					  type.bare_equal(object.type_info()) ||
					  object.type_info().bare_equal(function_type_info) ||
					  conversion->is_convertible_type(type, object.type_info())))) { return true; }
				return false;
			}

			/**
			 * @throw arity_error
			 */
			[[nodiscard]] boxed_value operator()(const parameters_view_type params, const type_conversion_state& conversion) const
			{
				if (arity_ < 0 || static_cast<decltype(params.size())>(arity_) == params.size()) { return do_invoke(params, conversion); }
				throw exception::arity_error{static_cast<arity_size_type>(params.size()), arity_};
			}

			/**
			 * @brief The number of arguments the function takes or -1(no_parameters_arity) if it is variadic.
			 */
			[[nodiscard]] constexpr arity_size_type get_arity() const noexcept { return arity_; }

			/**
			 * @brief Returns a vector containing all of the types of the parameters
			 * the function returns/takes. if the function is variadic or takes no arguments
			 * (arity of 0 or -1), the returned value contains exactly 1 gal_type_info object: the return type
			 *
			 * @return the types of all parameters.
			 */
			[[nodiscard]] const type_infos_type& types() const noexcept { return types_; }

			[[nodiscard]] constexpr bool has_arithmetic_param() const noexcept { return has_arithmetic_param_; }

			/**
			 * @brief Return true if the function is a possible match to the passed in values.
			 */
			[[nodiscard]] bool filter(const parameters_view_type params, const type_conversion_state& conversion) const noexcept
			{
				gal_assert(arity_ == no_parameters_arity || (arity_ > 0 && static_cast<arity_size_type>(params.size()) == arity_));

				if (arity_ < 0) { return true; }

				bool result = is_convertible_type(types_[1], params[0], conversion);

				if (arity_ > 1) { result &= is_convertible_type(types_[2], params[1], conversion); }

				return result;
			}

			[[nodiscard]] constexpr virtual bool is_member_function() const noexcept { return false; }

			/**
			 * @return const_proxy_function vector
			 */
			[[nodiscard]] virtual immutable_proxy_functions_type container_functions() const { return {}; }

			[[nodiscard]] virtual bool operator==(const proxy_function_base& other) const noexcept = 0;
			[[nodiscard]] virtual bool match(parameters_view_type params, const type_conversion_state& conversion) const = 0;

			[[nodiscard]] virtual bool is_first_type_match(const boxed_value& object, const type_conversion_state& conversion) const noexcept
			{
				gal_assert(types_.size() >= 2);
				return is_convertible_type(types_[1], object, conversion);
			}
		};

		/**
		 * @brief A proxy_function implementation that is not type safe, the called
		 * function is expecting a std::vector<boxed_value> that it works with how it chooses.
		 */
		class dynamic_proxy_function_base : public proxy_function_base
		{
		public:
			using parse_ast_node_type = std::shared_ptr<lang::ast_node_base>;

		private:
			parse_ast_node_type parse_ast_node_;
			proxy_function guard_;

		protected:
			parameter_type_mapper mapper_;

		private:
			static type_infos_type build_param_type_list(const parameter_type_mapper& types)
			{
				std::vector ret{make_type_info<boxed_value>()};

				for (const auto& ti: types.get_mapping() | std::views::values)
				{
					if (ti.is_undefined()) { ret.push_back(make_type_info<boxed_value>()); }
					else { ret.push_back(ti); }
				}

				return ret;
			}

		protected:
			[[nodiscard]] bool test_guard(const parameters_view_type params, const type_conversion_state& conversion) const
			{
				if (guard_)
				{
					try { return boxed_cast<bool>(guard_->operator()(params, conversion)); }
					catch (const exception::arity_error&) { return false; }
					catch (const exception::bad_boxed_cast&) { return false; }
				}
				return true;
			}

			/**
			 * @return pair.first means 'is match or not', pair.second means 'needs conversions'
			 */
			[[nodiscard]] std::pair<bool, bool> do_match(const parameters_view_type params, const type_conversion_state& conversion) const
			{
				// ReSharper disable once CppVariableCanBeMadeConstexpr
				const auto [m, c] = [&]
				{
					if (arity_ < 0) { return std::make_pair(true, false); }
					if (static_cast<decltype(params.size())>(arity_) == params.size()) { return mapper_.match(params, conversion); }
					return std::make_pair(false, false);
				}();

				return {m && test_guard(params, conversion), c};
			}

		public:
			dynamic_proxy_function_base(
					const arity_size_type arity,
					parse_ast_node_type&& node,
					parameter_type_mapper&& mapper = {},
					proxy_function&& guard = {})
				: proxy_function_base{build_param_type_list(mapper), arity},
				  parse_ast_node_{std::move(node)},
				  guard_{std::move(guard)},
				  mapper_{std::move(mapper)} {}

			[[nodiscard]] bool has_parse_tree() const { return parse_ast_node_.operator bool(); }

			[[nodiscard]] const parse_ast_node_type::element_type& get_parse_tree() const
			{
				if (parse_ast_node_) { return *parse_ast_node_; }
				throw std::runtime_error{"dynamic_proxy_function does not contain a parse_tree"};
			}

			[[nodiscard]] bool has_guard() const noexcept { return guard_.operator bool(); }

			[[nodiscard]] proxy_function get_guard() const noexcept { return guard_; }

			[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override
			{
				return this == &other ||
				       [&]
				       {
					       const auto* rhs = dynamic_cast<const dynamic_proxy_function_base*>(&other);

					       return rhs != nullptr &&
					              arity_ == rhs->arity_ &&
					              not has_guard() &&
					              not rhs->has_guard() &&
					              mapper_ == rhs->mapper_;
				       }();
			}

			[[nodiscard]] bool match(const parameters_view_type params, const type_conversion_state& conversion) const override { return do_match(params, conversion).first; }
		};

		template<typename Callable>
			requires(std::is_invocable_v<Callable, parameters_view_type> || std::is_invocable_v<Callable, parameters_type>)
		class dynamic_proxy_function final : public dynamic_proxy_function_base
		{
		private:
			Callable function_;

			[[nodiscard]] boxed_value do_invoke(parameters_view_type params, const type_conversion_state& conversion) const override
			{
				if (const auto [m, c] = do_match(params, conversion);
					m)
				{
					if (c)
					{
						if constexpr (std::is_invocable_v<Callable, parameters_view_type>)
						{
							// note that the argument is a tmp view
							return function_(parameters_view_type{mapper_.convert(params, conversion)});
						}
						else { return function_(mapper_.convert(params, conversion)); }
					}
					return function_(params);
				}
				throw exception::guard_error{};
			}

		public:
			dynamic_proxy_function(
					Callable&& function,
					const arity_size_type arity,
					parse_ast_node_type&& node,
					parameter_type_mapper&& mapper,
					proxy_function&& guard)
				: dynamic_proxy_function_base{arity, std::move(node), std::move(mapper), std::move(guard)},
				  function_{std::move(function)} {}
		};

		template<typename Callable, typename... Args>
		[[nodiscard]] proxy_function make_dynamic_proxy_function(Callable&& function, Args&&... args)
		{
			return std::make_shared<dynamic_proxy_function<Callable>>(
					std::forward<Callable>(function),
					std::forward<Args>(args)...);
		}

		/**
		 * @brief An object used by bound_function to represent "_" parameters
		 * of a binding. This allows for unbound parameters during bind.
		 */
		struct function_argument_placeholder { };

		/**
		 * @brief An implementation of proxy_function that takes a proxy_function
		 * and substitutes bound parameters into the parameter list
		 *  at runtime, when call() is executed.
		 *
		 *	@note it is used for bind(function, param1, _, param2) style calls
		 */
		class bound_function final : public proxy_function_base
		{
		private:
			immutable_proxy_function function_;
			parameters_type arguments_;

			static type_infos_type build_param_type_info(const immutable_proxy_function& function, const parameters_view_type arguments)
			{
				gal_assert(function->get_arity() < 0 || function->get_arity() == static_cast<arity_size_type>(arguments.size()));

				if (function->get_arity() < 0) { return {}; }

				const auto& types = function->types();
				gal_assert(types.size() == arguments.size() + 1);

				type_infos_type ret{types[0]};

				for (decltype(arguments.size()) i = 0; i < arguments.size(); ++i) { if (arguments[i].type_info() == make_type_info<function_argument_placeholder>()) { ret.push_back(types[i + 1]); } }

				return ret;
			}

			[[nodiscard]] boxed_value do_invoke(const parameters_view_type params, const type_conversion_state& conversion) const override { return (*function_)(parameters_view_type{build_parameters_list(params)}, conversion); }

		public:
			bound_function(
					immutable_proxy_function&& function,
					parameters_type&& arguments)
				: proxy_function_base{
						  build_param_type_info(function, parameters_view_type{arguments}),
						  function->get_arity() < 0 ? no_parameters_arity : static_cast<arity_size_type>(build_param_type_info(function, parameters_view_type{arguments}).size()) - 1},
				  function_{std::move(function)},
				  arguments_{std::move(arguments)} { gal_assert(function_->get_arity() < 0 || function_->get_arity() == static_cast<arity_size_type>(arguments_.size())); }

			[[nodiscard]] parameters_type build_parameters_list(const parameters_view_type params) const
			{
				return [it_param = params.begin(),
							it_arg = arguments_.begin(),

							end_param = params.end(),
							end_arg = arguments_.end()]() mutable
						{
							parameters_type ret{};

							while (not(it_param == end_param && it_arg == end_arg))
							{
								while (it_arg != end_arg && it_arg->type_info() != make_type_info<function_argument_placeholder>())
								{
									ret.push_back(*it_arg);
									++it_arg;
								}

								if (it_param != end_param)
								{
									ret.push_back(*it_param);
									++it_param;
								}

								if (it_arg != end_arg && it_arg->type_info() == make_type_info<function_argument_placeholder>()) { ++it_arg; }
							}

							return ret;
						}();
			}

			[[nodiscard]] immutable_proxy_functions_type container_functions() const override { return {function_}; }

			[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override { return &other == this; }

			[[nodiscard]] bool match(const parameters_view_type params, const type_conversion_state& conversion) const override { return function_->match(parameters_view_type{build_parameters_list(params)}, conversion); }
		};

		class proxy_function_addition_base : public proxy_function_base
		{
		public:
			explicit proxy_function_addition_base(
					type_infos_type&& types)
				: proxy_function_base{std::move(types), static_cast<arity_size_type>(types.size()) - 1} {}

			[[nodiscard]] bool match(const parameters_view_type params, const type_conversion_state& conversion) const override { return get_arity() == static_cast<arity_size_type>(params.size()) && is_convertible_types(type_infos_view_type{types_}, params, conversion) && is_invokable(params, conversion); }

			[[nodiscard]] virtual bool is_invokable(parameters_view_type params, const type_conversion_state& conversion) const noexcept = 0;
		};

		/**
		 * @brief For any callable object
		 */
		template<typename FunctionSignature, typename Callable>
		class proxy_function_callable final : public proxy_function_addition_base
		{
		private:
			Callable function_;

			[[nodiscard]] boxed_value do_invoke(parameters_view_type params, const type_conversion_state& conversion) const override { return proxy_function_detail::do_invoke(static_cast<FunctionSignature*>(nullptr), function_, params, conversion); }

		public:
			explicit proxy_function_callable(
					Callable&& function)
				: proxy_function_addition_base{proxy_function_detail::build_params_type_list(static_cast<FunctionSignature*>(nullptr))},
				  function_{std::move(function)} {}

			[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override { return dynamic_cast<const proxy_function_callable<FunctionSignature, Callable>*>(&other); }

			[[nodiscard]] bool is_invokable(const parameters_view_type params, const type_conversion_state& conversion) const noexcept override { return proxy_function_detail::is_invokable(static_cast<FunctionSignature*>(nullptr), params, conversion); }
		};

		class proxy_function_assignable_base : public proxy_function_addition_base
		{
		public:
			explicit proxy_function_assignable_base(
					type_infos_type&& types)
				: proxy_function_addition_base{std::move(types)} {}

			virtual void assign(const immutable_proxy_function& other) = 0;
		};

		template<typename FunctionSignature>
		class proxy_function_assignable final : public proxy_function_assignable_base
		{
		private:
			std::reference_wrapper<std::function<FunctionSignature>> function_;
			std::shared_ptr<std::function<FunctionSignature>> shared_function_;

			[[nodiscard]] boxed_value do_invoke(parameters_view_type params, const type_conversion_state& conversion) const override { return proxy_function_detail::do_invoke(static_cast<FunctionSignature*>(nullptr), function_.get(), params, conversion); }

		public:
			proxy_function_assignable(
					std::reference_wrapper<std::function<FunctionSignature>>&& function,
					std::shared_ptr<std::function<FunctionSignature>>&& shared_function)
				: proxy_function_assignable_base{proxy_function_detail::build_params_type_list(static_cast<FunctionSignature*>(nullptr))},
				  function_{std::move(function)},
				  shared_function_{std::move(shared_function)} { gal_assert(not shared_function || shared_function.get() == &function.get()); }

			[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override { return dynamic_cast<const proxy_function_assignable<FunctionSignature>*>(&other); }

			[[nodiscard]] bool is_invokable(const parameters_view_type params, const type_conversion_state& conversion) const noexcept override { return proxy_function_detail::is_invokable(static_cast<FunctionSignature*>(nullptr), params, conversion); }

			void assign(const immutable_proxy_function& other) override { function_.get() = make_functor<FunctionSignature>(other, nullptr); }
		};

		template<typename T, typename Class>
		class member_accessor final : public proxy_function_base
		{
		public:
			constexpr static arity_size_type arity_size = 1;

		private:
			T Class::* member_;

			static type_infos_type build_param_types() { return {make_type_info<T>(), make_type_info<Class>()}; }

			template<typename U>
			auto do_invoke(Class* object) const
			{
				if constexpr (std::is_pointer_v<U>) { return return_handler_detail::return_handler<U>::handle(object->*member_); }
				else { return return_handler_detail::return_handler<std::add_lvalue_reference_t<U>>::handle(object->*member_); }
			}

			template<typename U>
			auto do_invoke(const Class* object) const
			{
				if constexpr (std::is_pointer_v<U>) { return return_handler_detail::return_handler<const U>::handle(object->*member_); }
				else { return return_handler_detail::return_handler<std::add_lvalue_reference_t<std::add_const_t<U>>>::handle(object->*member_); }
			}

			[[nodiscard]] boxed_value do_invoke(const parameters_view_type params, const type_conversion_state& conversion) const override
			{
				const auto& object = params.front();

				if (object.is_const()) { return do_invoke<T>(boxed_cast<const Class*>(object, &conversion)); }
				return do_invoke<T>(boxed_cast<Class*>(object, &conversion));
			}

		public:
			explicit member_accessor(T Class::* member)
				: proxy_function_base{build_param_types(), arity_size},
				  member_{member} {}

			[[nodiscard]] constexpr bool is_member_function() const noexcept override { return true; }

			[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override
			{
				if (const auto* accessor = dynamic_cast<const member_accessor<T, Class>*>(&other)) { return member_ == accessor->member_; }
				return false;
			}

			[[nodiscard]] bool match(const parameters_view_type params, const type_conversion_state&) const override
			{
				if (arity_size != static_cast<arity_size_type>(params.size())) { return false; }

				return params.front().type_info().bare_equal(make_type_info<Class>());
			}
		};

		namespace proxy_function_detail
		{
			[[nodiscard]] inline bool types_match_except_for_arithmetic(
					const proxy_function_base& function,
					const parameters_view_type params,
					const type_conversion_state& conversion)
			{
				if (function.get_arity() == proxy_function_base::no_parameters_arity) { return false; }

				const auto& types = function.types();
				gal_assert(params.size() == types.size() - 1);

				return std::mismatch(
						       params.begin(),
						       params.end(),
						       types.begin() + 1,
						       [&](const auto& object, const auto& type) { return proxy_function_base::is_convertible_type(type, object, conversion) || (object.type_info().is_arithmetic() && type.is_arithmetic()); }) == std::make_pair(params.end(), types.end());
			}

			template<typename Functions>
			[[nodiscard]] boxed_value dispatch_with_conversion(
					const std::ranges::range auto& range,
					const parameters_view_type parameters,
					const type_conversion_state& conversion,
					const Functions& functions)
			{
				const auto end = std::ranges::end(range);
				auto matching = end;

				for (auto begin = std::ranges::begin(range); begin != end; ++begin)
				{
					if (types_match_except_for_arithmetic(*begin, parameters, conversion))
					{
						if (matching == end) { matching = begin; }
						else
						{
							// handle const members vs non-const member, which is not really ambiguous
							const auto& match_function_param_types = (*matching).types();
							const auto& next_function_param_types = (*begin).types();

							if (
								parameters.front().is_const() &&
								not match_function_param_types[1].is_const() &&
								next_function_param_types[1].is_const())
							{
								// keep the new one, the const/non-const match up is correct
								matching = begin;
							}
							else if (
								not parameters.front().is_const() &&
								not match_function_param_types[1].is_const() &&
								next_function_param_types[1].is_const())
							{
								// keep the old one, it has a better const/non-const match up
								// do nothing
							}
							else
							{
								// ambiguous function call
								throw exception::dispatch_error{
										parameters.to<parameters_type>(),
										immutable_proxy_functions_type{
												functions.begin(),
												functions.end()}};
							}
						}
					}
				}

				if (matching == end)
				{
					// no appropriate function to attempt arithmetic type conversion on
					throw exception::dispatch_error{
							parameters.to<parameters_type>(),
							immutable_proxy_functions_type{
									functions.begin(),
									functions.end()}};
				}

				parameters_type new_parameters;
				new_parameters.reserve(parameters.size());

				const auto& tis = (*matching).types();

				std::transform(
						tis.begin() + 1,
						tis.end(),
						parameters.begin(),
						std::back_inserter(new_parameters),
						[](const auto& type, const auto& param) -> boxed_value
						{
							if (type.is_arithmetic() && param.type_info().is_arithmetic() && param.type_info() != type) { return boxed_number{param}.as(type).value; }
							return param;
						});

				try { return (*matching)(parameters_view_type{new_parameters}, conversion); }
				catch (const gal::lang::exception::bad_boxed_cast&)
				{
					// parameter failed to cast
				}
				catch (const gal::lang::exception::arity_error&)
				{
					// invalid num params
				}
				catch (const gal::lang::exception::guard_error&)
				{
					// guard failed to allow the function to execute
				}

				throw exception::dispatch_error{
						parameters.to<parameters_type>(),
						immutable_proxy_functions_type{
								functions.begin(),
								functions.end()}};
			}
		}// namespace proxy_function_detail
	}    // namespace foundation
}        // namespace gal::lang

#endif// GAL_LANG_FOUNDATION_PROXY_FUNCTION_HPP
