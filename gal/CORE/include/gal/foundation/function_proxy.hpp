#pragma once

#ifndef GAL_LANG_FOUNDATION_FUNCTION_PROXY_HPP
#define GAL_LANG_FOUNDATION_FUNCTION_PROXY_HPP

#include <gal/boxed_cast.hpp>
#include <gal/foundation/dynamic_object.hpp>
#include <gal/foundation/parameters.hpp>
#include <gal/foundation/return_wrapper.hpp>
#include <gal/language/name.hpp>
#include <gal/tools/logger.hpp>
#include <memory>
#include <ranges>

namespace gal::lang
{
	namespace lang
	{
		struct ast_node;
	}// namespace lang

	namespace exception
	{
		/**
		 * @brief Exception thrown when there is a mismatch in number of
		 * parameters during function_proxy execution.
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
			foundation::const_function_proxies_type functions;

			dispatch_error(
					foundation::parameters_type parameters,
					foundation::const_function_proxies_type functions,
					const std::string_view message = "Error with function dispatch")
				: std::runtime_error{message.data()},
				  parameters{std::move(parameters)},
				  functions{std::move(functions)} {}
		};
	}// namespace exception

	namespace foundation
	{
		namespace function_proxy_detail
		{
			/**
			 * @brief Used by function_proxy to return a list of all param types it contains.
			 */
			template<typename Result, typename... Params>
			[[nodiscard]] type_infos_type build_params_type_list(Result (*)(Params ...)) { return {make_type_info<Result>(), make_type_info<Params>()...}; }

			/**
			 * @brief Used by function_proxy to determine if it is equivalent to another
			 * function_proxy object. This function is primarily used to prevent registration
			 * of two functions with the exact same signatures.
			 */
			template<typename Result, typename... Params>
			bool is_invokable(
					Result (*)(Params ...),
					const parameters_view_type params,
					const convertor_manager_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())
					) noexcept
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), with {} params.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							params.size());)

				try
				{
					[&, i = parameters_type::size_type{0}]() mutable { ((void)boxed_cast<Params>(params[i++], &state), ...); }();

					return true;
				}
				catch (const exception::bad_boxed_cast&) { return false; }
			}

			/**
			 * @brief Used by function_proxy to perform type safe execution of a function.
			 * @note The function attempts to unbox each parameter to the expected type.
			 * if any unboxing fails the execution of the function fails and
			 * the bad_boxed_cast is passed up to the caller.
			 */
			template<typename Callable, typename Result, typename... Params>
			boxed_value do_invoke(
					Result (*)(Params ...),
					const Callable& function,
					const parameters_view_type params,
					const convertor_manager_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())
					)
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), with {} params.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							params.size());)

				auto call = [&]<bool HasReturn, std::size_t... Index>(std::index_sequence<Index...>)-> std::conditional_t<HasReturn, Result, void>
				{
					if constexpr (HasReturn) { return function(boxed_cast<Params>(params[Index], &state)...); }
					else { return function(boxed_cast<Params>(params[Index], &state)...); }
				};

				if constexpr (std::is_same_v<Result, void>)
				{
					call.decltype(call)::template operator()<false>(std::index_sequence_for<Params...>{});
					return return_wrapper_detail::return_wrapper<void>::wrapper();
				}
				else { return return_wrapper_detail::return_wrapper<Result>::handle(call.decltype(call)::template operator()<true>(std::index_sequence_for<Params...>{})); }
			}
		}

		class parameter_type_mapper
		{
		public:
			using parameter_type_mapping_type = std::vector<std::pair<string_view_type, gal_type_info>>;

		private:
			parameter_type_mapping_type mapping_;

		public:
			parameter_type_mapper() = default;

			explicit parameter_type_mapper(parameter_type_mapping_type mapping)
				: mapping_{std::move(mapping)} {}

			template<typename... Args>
				requires std::is_constructible_v<parameter_type_mapping_type, Args...>
			explicit parameter_type_mapper(Args&&... args)
				: mapping_{std::forward<Args>(args)...} {}

			[[nodiscard]] bool operator==(const parameter_type_mapper& other) const { return mapping_ == other.mapping_; }

			void add(const string_view_type name, const gal_type_info& type) { mapping_.emplace_back(name, type); }

			void inplace_convert(
					parameters_type& params,
					const convertor_manager_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) const
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), with {} params.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							params.size());)

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						string_type detail{};
						std::ranges::for_each(mapping_ | std::views::keys, [&detail](const auto& name)
							{ detail.append(name).push_back('\n'); });
						tools::logger::debug("mapper details:\n {}", detail);
						)

				for (decltype(params.size()) i = 0; i < params.size(); ++i)
				{
					// the newest is last, so from back to front
					if (const auto& [name, type] = mapping_[mapping_.size() - 1 - i];
						not name.empty())
					{
						if (const auto& object = params[i];
							not object.type_info().bare_equal(dynamic_object::class_type()))
						{
							if (not type.is_undefined())
							{
								if (not object.type_info().bare_equal(type))
								{
									if (state->is_convertible(type, object.type_info()))
									{
										try
										{
											// We will not catch any bad_boxed_cast that is thrown, let the user get it
											// either way, we are not responsible if it doesn't work
											params[i] = state->boxed_convert(object, type);
										}
										catch (...)
										{
											try
											{
												// try going the other way
												params[i] = state->boxed_convert_down(type, object);
											}
											catch (const std::bad_any_cast&) { throw exception::bad_boxed_cast{object.type_info(), type.bare_type_info()}; }
										}
									}
								}
							}
						}
					}
				}
			}

			[[nodiscard]] parameters_type convert(
					const parameters_view_type& params,
					const convertor_manager_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())
					) const
			{
				auto ret = params.to<parameters_type>();
				inplace_convert(ret, state GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, location));
				return ret;
			}

			/**
			 * @return pair.first means 'is match or not', pair.second means 'needs conversions'
			 */
			[[nodiscard]] std::pair<bool, bool> match(const parameters_view_type params, const convertor_manager_state& state) const noexcept
			{
				bool need_conversion = false;

				if (params.size() != mapping_.size()) { return {false, need_conversion}; }

				for (decltype(params.size()) i = 0; i < params.size(); ++i)
				{
					// the newest is last, so from back to front
					if (const auto& [name, type] = mapping_[mapping_.size() - 1 - i];
						not name.empty())
					{
						if (const auto& object = params[i];
							object.type_info().bare_equal(dynamic_object::class_type()))
						{
							try
							{
								if (not(lang::dynamic_object_type_name::match(name) ||
								        [&] { return boxed_cast<const dynamic_object&>(object, &state); }().nameof() == name)) { return {false, false}; }
							}
							catch (const std::bad_cast&) { return {false, false}; }
						}
						else
						{
							if (not type.is_undefined())
							{
								if (not object.type_info().bare_equal(type))
								{
									if (not state->is_convertible(object.type_info(), type)) { return {false, false}; }
									need_conversion = true;
								}
							}
							else { return {false, false}; }
						}
					}
				}

				return {true, need_conversion};
			}

			[[nodiscard]] constexpr auto view() const noexcept { return mapping_ | std::views::all; }
		};

		/**
		 * @brief Pure virtual base class for all function_proxy implementations.
		 * function_proxy are a type erasure of type safe C++ function calls.
		 * At runtime parameter types are expected to be tested against passed in types.
		 * dispatcher only knows how to work with function_proxy, no other
		 * function classes.
		 */
		class function_proxy_base
		{
		public:
			function_proxy_base() = default;
			function_proxy_base(const function_proxy_base&) = default;
			function_proxy_base& operator=(const function_proxy_base&) = default;
			function_proxy_base(function_proxy_base&&) = default;
			function_proxy_base& operator=(function_proxy_base&&) = default;
			virtual ~function_proxy_base() noexcept = default;

			static_assert(std::is_signed_v<exception::arity_error::size_type>);
			using arity_size_type = exception::arity_error::size_type;
			constexpr static arity_size_type no_parameters_arity = -1;

		protected:
			type_infos_type types_;
			arity_size_type arity_{};
			bool has_arithmetic_param_{};

			static bool is_all_convertible(const type_infos_view_type types, const parameters_view_type params, const convertor_manager_state& state) noexcept
			{
				if (params.size() + 1 != types.size()) { return false; }

				for (decltype(params.size()) i = 0; i < params.size(); ++i) { if (not is_convertible(types[i + 1], params[i], state)) { return false; } }

				return true;
			}

			function_proxy_base(type_infos_type types, const arity_size_type arity)
				: types_{std::move(types)},
				  arity_{arity},
				  has_arithmetic_param_{std::ranges::any_of(types_, [](const auto& type) { return type.is_arithmetic(); })} {}

		private:
			[[nodiscard]] virtual boxed_value do_invoke(parameters_view_type params, const convertor_manager_state& state) const = 0;

		public:
			[[nodiscard]] static bool is_convertible(const gal_type_info& type, const boxed_value& object, const convertor_manager_state& state) noexcept
			{
				const auto function_type_info = make_type_info<const_function_proxies_type::value_type>();

				if (
					type.is_undefined() ||
					type.bare_equal(boxed_value::class_type()) ||
					(not object.type_info().is_undefined() &&
					 ((type.bare_equal(boxed_number::class_type()) && object.type_info().is_arithmetic()) ||
					  type.bare_equal(object.type_info()) ||
					  object.type_info().bare_equal(function_type_info) ||
					  state->is_convertible(object.type_info(), type)))) { return true; }
				return false;
			}

			/**
			 * @throw exception::arity_error
			 */
			[[nodiscard]] boxed_value operator()(
					const parameters_view_type params,
					const convertor_manager_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())
					) const
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), try to invoke a function_proxy(arity = '{}') with '{}' params.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							arity_,
							params.size());)

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						string_type detail{};
						std::ranges::for_each(params, [&detail](const auto& object)
							{ std_format::format_to(
								std::back_inserter(detail),
								"type: '{}({})'\n",
								object.type_info().type_name,
								object.type_info().bare_type_name); });
						tools::logger::debug("params details:\n {}", detail);
						)

				if (arity_ < 0 || static_cast<decltype(params.size())>(arity_) == params.size()) { return do_invoke(params, state); }

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), invoke a function_proxy(arity = '{}') with '{}' params failed.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							arity_,
							params.size());)

				throw exception::arity_error{static_cast<arity_size_type>(params.size()), arity_};
			}

			/**
			 * @throw exception::arity_error
			 */
			[[nodiscard]] boxed_value operator()(
					const parameters_type& params,
					const convertor_manager_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) const { return this->operator()(parameters_view_type{params}, state GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, location)); }

			/**
			 * @brief The number of arguments the function takes or -1(no_parameters_arity) if it is variadic.
			 */
			[[nodiscard]] constexpr arity_size_type arity_size() const noexcept { return arity_; }

			/**
			 * @brief Returns a view containing all of the types of the parameters
			 * the function returns/takes. if the function is variadic or takes no arguments
			 * (arity of 0 or -1), the returned value contains exactly 1 gal_type_info object: the return type
			 *
			 * @return the types of all parameters.
			 */
			[[nodiscard]] constexpr auto type_view() const noexcept { return types_ | std::views::all; }

			[[nodiscard]] constexpr bool has_arithmetic_param() const noexcept { return has_arithmetic_param_; }

			/**
			 * @brief Return true if the function is a possible match to the passed in values.
			 */
			[[nodiscard]] bool filter(const parameters_view_type params, const convertor_manager_state& state) const noexcept
			{
				gal_assert(arity_ == no_parameters_arity || (arity_ > 0 && static_cast<arity_size_type>(params.size()) == arity_));

				if (arity_ < 0) { return true; }

				bool result = is_convertible(types_[1], params[0], state);

				if (arity_ > 1) { result &= is_convertible(types_[2], params[1], state); }

				return result;
			}

			[[nodiscard]] constexpr virtual bool is_member_function() const noexcept { return false; }

			/**
			 * @brief Get all overloaded function (with the same name).
			 *
			 * @return const_proxy_function vector
			 */
			[[nodiscard]] virtual const_function_proxies_type overloaded_functions() const { return {}; }

			[[nodiscard]] virtual bool operator==(const function_proxy_base& other) const noexcept = 0;
			[[nodiscard]] virtual bool match(parameters_view_type params, const convertor_manager_state& state) const = 0;

			[[nodiscard]] virtual bool is_first_type_match(const boxed_value& object, const convertor_manager_state& state) const noexcept
			{
				gal_assert(types_.size() >= 2);
				return is_convertible(types_[1], object, state);
			}
		};

		/**
		 * @brief A proxy_function implementation that is not type safe, the called
		 * function is expecting a parameters_view_type that it works with how it chooses.
		 */
		class dynamic_function_proxy_base : public function_proxy_base
		{
		public:
			using body_block_type = std::shared_ptr<lang::ast_node>;

		private:
			body_block_type function_body_;
			function_proxy_type guard_;

		protected:
			parameter_type_mapper mapper_;

		private:
			[[nodiscard]] static type_infos_type build_param_type_list(const parameter_type_mapper& types)
			{
				std::vector ret{boxed_value::class_type()};

				for (const auto& type: types.view() | std::views::values)
				{
					if (type.is_undefined()) { ret.push_back(boxed_value::class_type()); }
					else { ret.push_back(type); }
				}

				return ret;
			}

		protected:
			[[nodiscard]] bool test_guard(const parameters_view_type params, const convertor_manager_state& state) const
			{
				if (guard_)
				{
					try { return boxed_cast<bool>(guard_->operator()(params, state)); }
					catch (const exception::arity_error&) { return false; }
					catch (const exception::bad_boxed_cast&) { return false; }
				}
				return true;
			}

			/**
			 * @return pair.first means 'is match or not', pair.second means 'needs conversions'
			 */
			[[nodiscard]] std::pair<bool, bool> do_match(const parameters_view_type params, const convertor_manager_state& state) const
			{
				// ReSharper disable once CppVariableCanBeMadeConstexpr
				const auto [m, c] = [&]
				{
					if (arity_ < 0) { return std::make_pair(true, false); }
					if (static_cast<decltype(params.size())>(arity_) == params.size()) { return mapper_.match(params, state); }
					return std::make_pair(false, false);
				}();

				return {m && test_guard(params, state), c};
			}

		public:
			dynamic_function_proxy_base(
					const arity_size_type arity,
					body_block_type body_node,
					parameter_type_mapper&& mapper = {},
					function_proxy_type&& guard = {})
				: function_proxy_base{build_param_type_list(mapper), arity},
				  function_body_{std::move(body_node)},
				  guard_{std::move(guard)},
				  mapper_{std::move(mapper)} {}

			[[nodiscard]] bool has_function_body() const { return function_body_.operator bool(); }

			[[nodiscard]] const body_block_type::element_type& get_function_body() const
			{
				if (function_body_) { return *function_body_; }
				throw std::runtime_error{"dynamic_function_proxy does not contain a function body"};
			}

			[[nodiscard]] bool has_guard() const noexcept { return guard_.operator bool(); }

			[[nodiscard]] function_proxy_type get_guard() const noexcept { return guard_; }

			[[nodiscard]] bool operator==(const function_proxy_base& other) const noexcept override
			{
				return this == &other ||
				       [&]
				       {
					       const auto* rhs = dynamic_cast<const dynamic_function_proxy_base*>(&other);

					       return rhs != nullptr &&
					              arity_ == rhs->arity_ &&
					              not has_guard() &&
					              not rhs->has_guard() &&
					              mapper_ == rhs->mapper_;
				       }();
			}

			[[nodiscard]] bool match(const parameters_view_type params, const convertor_manager_state& state) const override { return do_match(params, state).first; }
		};

		template<typename Callable>
			requires(std::is_invocable_v<Callable, const parameters_view_type> || std::is_invocable_v<Callable, const parameters_type&>)
		class dynamic_function_proxy final : public dynamic_function_proxy_base
		{
		public:
			using callable_type = Callable;

		private:
			callable_type function_;

			[[nodiscard]] boxed_value do_invoke(const parameters_view_type params, const convertor_manager_state& state) const override
			{
				if (const auto [m, c] = do_match(params, state);
					m)
				{
					if (c)
					{
						if constexpr (std::is_invocable_v<Callable, parameters_view_type>)
						{
							// note that the argument is a tmp view
							return function_(parameters_view_type{mapper_.convert(params, state)});
						}
						else { return function_(mapper_.convert(params, state)); }
					}
					return function_(params);
				}
				throw exception::guard_error{};
			}

		public:
			explicit dynamic_function_proxy(
					Callable&& function,
					const arity_size_type arity = no_parameters_arity,
					body_block_type body_node = {},
					parameter_type_mapper&& mapper = {},
					function_proxy_type&& guard = {})
				: dynamic_function_proxy_base{arity, std::move(body_node), std::move(mapper), std::move(guard)},
				  function_{std::move(function)} {}
		};

		// template<typename Callable, typename... Args>
		// [[nodiscard]] function_proxy_type make_dynamic_function_proxy(Callable&& function, Args&&... args)
		// {
		// 	return std::make_shared<dynamic_function_proxy<Callable>>(
		// 			std::forward<Callable>(function),
		// 			std::forward<Args>(args)...);
		// }

		template<typename Callable>
		[[nodiscard]] function_proxy_type make_dynamic_function_proxy(
				Callable&& function,
				const function_proxy_base::arity_size_type arity = function_proxy_base::no_parameters_arity,
				dynamic_function_proxy_base::body_block_type body_node = {},
				parameter_type_mapper&& mapper = {},
				function_proxy_type&& guard = {}
				)
		{
			return std::make_shared<dynamic_function_proxy<Callable>>(
					std::forward<Callable>(function),
					arity,
					std::move(body_node),
					std::move(mapper),
					std::move(guard));
		}

		/**
		 * @brief An object used by bound_function to represent "_" parameters
		 * of a binding. This allows for unbound parameters during bind.
		 */
		struct function_argument_placeholder final
		{
			GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,)static const gal_type_info& class_type() noexcept
			{
				GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,)static gal_type_info type = make_type_info<function_argument_placeholder>();
				return type;
			}
		};

		/**
		 * @brief An implementation of proxy_function that takes a proxy_function
		 * and substitutes bound parameters into the parameter list
		 *  at runtime, when call() is executed.
		 *
		 *	@note it is used for bind(function, param1, _, param2) style calls
		 */
		class bound_function final : public function_proxy_base
		{
		private:
			const_function_proxy_type function_;
			parameters_type arguments_;

			[[nodiscard]] static type_infos_type build_param_type_info(const const_function_proxy_type& function, const parameters_view_type arguments)
			{
				gal_assert(function->arity_size() < 0 || function->arity_size() == static_cast<arity_size_type>(arguments.size()));

				if (function->arity_size() < 0) { return {}; }

				const auto types = function->type_view();
				gal_assert(types.size() == arguments.size() + 1);

				type_infos_type ret{types[0]};

				for (decltype(arguments.size()) i = 0; i < arguments.size(); ++i) { if (arguments[i].type_info() == function_argument_placeholder::class_type()) { ret.push_back(types[static_cast<type_infos_type::difference_type>(i) + 1]); } }

				return ret;
			}

			[[nodiscard]] boxed_value do_invoke(const parameters_view_type params, const convertor_manager_state& state) const override { return (*function_)(build_parameters_list(params), state); }

		public:
			bound_function(
					const_function_proxy_type function,
					parameters_type&& arguments)
				: function_proxy_base{
						  build_param_type_info(function, arguments),
						  function->arity_size() < 0 ? no_parameters_arity : static_cast<arity_size_type>(build_param_type_info(function, arguments).size()) - 1},
				  function_{std::move(function)},
				  arguments_{std::move(arguments)} { gal_assert(function_->arity_size() < 0 || function_->arity_size() == static_cast<arity_size_type>(arguments_.size())); }

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
								while (it_arg != end_arg && it_arg->type_info() != function_argument_placeholder::class_type())
								{
									ret.push_back(*it_arg);
									++it_arg;
								}

								if (it_param != end_param)
								{
									ret.push_back(*it_param);
									++it_param;
								}

								if (it_arg != end_arg && it_arg->type_info() == function_argument_placeholder::class_type()) { ++it_arg; }
							}

							return ret;
						}();
			}

			[[nodiscard]] const_function_proxies_type overloaded_functions() const override { return {function_}; }


			[[nodiscard]] bool operator==(const function_proxy_base& other) const noexcept override { return &other == this; }

			[[nodiscard]] bool match(const parameters_view_type params, const convertor_manager_state& state) const override { return function_->match(build_parameters_list(params), state); }
		};

		class extra_function_proxy_base : public function_proxy_base
		{
		public:
			explicit extra_function_proxy_base(
					type_infos_type&& types)
				: function_proxy_base{std::move(types), static_cast<arity_size_type>(types.size()) - 1} {}

			[[nodiscard]] bool match(const parameters_view_type params, const convertor_manager_state& state) const override { return arity_size() == static_cast<arity_size_type>(params.size()) && is_all_convertible(types_, params, state) && is_invokable(params, state); }

			[[nodiscard]] virtual bool is_invokable(parameters_view_type params, const convertor_manager_state& state) const noexcept = 0;
		};

		/**
		 * @brief For any callable object
		 */
		template<typename FunctionSignature, typename Callable>
		class callable_function_proxy : public extra_function_proxy_base
		{
		public:
			using function_signature_type = FunctionSignature;
			using callable_type = Callable;

		private:
			callable_type function_;

			[[nodiscard]] boxed_value do_invoke(parameters_view_type params, const convertor_manager_state& state) const override { return function_proxy_detail::do_invoke(static_cast<function_signature_type*>(nullptr), function_, params, state); }

		public:
			explicit callable_function_proxy(
					Callable&& function)
				: extra_function_proxy_base{function_proxy_detail::build_params_type_list(static_cast<function_signature_type*>(nullptr))},
				  function_{std::move(function)} {}

			[[nodiscard]] bool operator==(const function_proxy_base& other) const noexcept override { return dynamic_cast<const callable_function_proxy<function_signature_type, callable_type>*>(&other); }

			[[nodiscard]] bool is_invokable(const parameters_view_type params, const convertor_manager_state& state) const noexcept override { return function_proxy_detail::is_invokable(static_cast<function_signature_type*>(nullptr), params, state); }
		};

		class assignable_function_proxy_base : public extra_function_proxy_base
		{
		public:
			explicit assignable_function_proxy_base(
					type_infos_type&& types)
				: extra_function_proxy_base{std::move(types)} {}

			virtual void assign(const const_function_proxy_type& other) = 0;
		};

		template<typename FunctionSignature>
		std::function<FunctionSignature> make_functor(
				const_function_proxy_type function,
				const convertor_manager_state* state);

		template<typename FunctionSignature>
		class assignable_function_proxy final : public assignable_function_proxy_base
		{
		public:
			using function_signature_type = FunctionSignature;
			using callable_type = std::reference_wrapper<std::function<function_signature_type>>;
			using shared_callable_type = std::shared_ptr<std::function<function_signature_type>>;

		private:
			callable_type function_;
			shared_callable_type shared_function_;

			[[nodiscard]] boxed_value do_invoke(parameters_view_type params, const convertor_manager_state& state) const override { return function_proxy_detail::do_invoke(static_cast<function_signature_type*>(nullptr), function_.get(), params, state); }

		public:
			assignable_function_proxy(
					callable_type function,
					shared_callable_type shared_function)
				: assignable_function_proxy_base{function_proxy_detail::build_params_type_list(static_cast<function_signature_type*>(nullptr))},
				  function_{std::move(function)},
				  shared_function_{std::move(shared_function)} { gal_assert(not shared_function || shared_function.get() == &function.get()); }

			[[nodiscard]] bool operator==(const function_proxy_base& other) const noexcept override { return dynamic_cast<const assignable_function_proxy<function_signature_type>*>(&other); }

			[[nodiscard]] bool is_invokable(const parameters_view_type params, const convertor_manager_state& state) const noexcept override { return function_proxy_detail::is_invokable(static_cast<function_signature_type*>(nullptr), params, state); }

			void assign(const const_function_proxy_type& other) override { function_.get() = make_functor<function_signature_type>(other, nullptr); }
		};

		template<typename DataType, typename ClassType>
		class member_accessor final : public function_proxy_base
		{
		public:
			using data_type = DataType;
			using class_type = ClassType;

			constexpr static arity_size_type arity_size = 1;

		private:
			data_type class_type::* member_;

			[[nodiscard]] static type_infos_type build_param_types() { return {make_type_info<data_type>(), make_type_info<class_type>()}; }

			template<typename U>
			auto do_invoke(class_type* object) const
			{
				if constexpr (std::is_pointer_v<U>) { return return_wrapper_detail::return_wrapper<U>::wrapper(object->*member_); }
				else { return return_wrapper_detail::return_wrapper<std::add_lvalue_reference_t<U>>::wrapper(object->*member_); }
			}

			template<typename U>
			auto do_invoke(const class_type* object) const
			{
				if constexpr (std::is_pointer_v<U>) { return return_wrapper_detail::return_wrapper<const U>::wrapper(object->*member_); }
				else { return return_wrapper_detail::return_wrapper<std::add_lvalue_reference_t<std::add_const_t<U>>>::wrapper(object->*member_); }
			}

			[[nodiscard]] boxed_value do_invoke(const parameters_view_type params, const convertor_manager_state& state) const override
			{
				const auto& object = params.front();

				if (object.is_const()) { return this->template do_invoke<data_type>(boxed_cast<const class_type*>(object, &state)); }
				return this->template do_invoke<data_type>(boxed_cast<class_type*>(object, &state));
			}

		public:
			explicit member_accessor(data_type class_type::* member)
				: function_proxy_base{build_param_types(), arity_size},
				  member_{member} {}

			[[nodiscard]] constexpr bool is_member_function() const noexcept override { return true; }

			[[nodiscard]] bool operator==(const function_proxy_base& other) const noexcept override
			{
				if (const auto* accessor = dynamic_cast<const member_accessor<data_type, class_type>*>(&other)) { return member_ == accessor->member_; }
				return false;
			}

			[[nodiscard]] bool match(const parameters_view_type params, const convertor_manager_state&) const override
			{
				if (arity_size != static_cast<arity_size_type>(params.size())) { return false; }

				return params.front().type_info().bare_equal(make_type_info<class_type>());
			}
		};

		namespace proxy_function_detail
		{
			[[nodiscard]] inline bool types_match_except_for_arithmetic(
					const function_proxy_base& function,
					const parameters_view_type params,
					const convertor_manager_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())
					)
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), try to match a function_proxy(arity = '{}') with '{}' params.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							function.arity_size(),
							params.size());)

				if (function.arity_size() == function_proxy_base::no_parameters_arity) { return false; }

				const auto types = function.type_view();
				gal_assert(params.size() == types.size() - 1);

				return std::mismatch(
						       params.begin(),
						       params.end(),
						       types.begin() + 1,
						       [&](const auto& object, const auto& type) { return function_proxy_base::is_convertible(type, object, state) || (object.type_info().is_arithmetic() && type.is_arithmetic()); }) == std::make_pair(params.end(), types.end());
			}

			template<typename Functions>
			[[nodiscard]] boxed_value dispatch_with_conversion(
					const std::ranges::range auto& range,
					const parameters_view_type params,
					const convertor_manager_state& conversion,
					const Functions& functions
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())
					)
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), try to dispatch a bunch of functions(size: '{}') with '{}' params.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							std::ranges::size(range),
							params.size());)

				const auto end = std::ranges::end(range);
				auto matching = end;

				for (auto begin = std::ranges::begin(range); begin != end; ++begin)
				{
					if (proxy_function_detail::types_match_except_for_arithmetic(*begin, params, conversion))
					{
						GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
								tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), types_match_except_for_arithmetic matched at the '{}'th function.",
									__func__,
									location.file_name(),
									location.function_name(),
									location.line(),
									location.column(),
									std::ranges::distance(std::ranges::begin(range), begin));)

						if (matching == end) { matching = begin; }
						else
						{
							// handle const members vs non-const member, which is not really ambiguous
							const auto& match_function_param_types = (*matching).type_view();
							const auto& next_function_param_types = (*begin).type_view();

							if (
								params.front().is_const() &&
								not match_function_param_types[1].is_const() &&
								next_function_param_types[1].is_const())
							{
								// keep the new one, the const/non-const match up is correct
								matching = begin;
							}
							else if (
								not params.front().is_const() &&
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
										params.to<parameters_type>(),
										const_function_proxies_type{
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
							params.to<parameters_type>(),
							const_function_proxies_type{
									functions.begin(),
									functions.end()}};
				}

				parameters_type new_parameters;
				new_parameters.reserve(params.size());

				const auto& tis = (*matching).type_view();

				std::transform(
						tis.begin() + 1,
						tis.end(),
						params.begin(),
						std::back_inserter(new_parameters),
						[](const auto& type, const auto& param) -> boxed_value
						{
							if (type.is_arithmetic() && param.type_info().is_arithmetic() && param.type_info() != type) { return boxed_number{param}.as(type).value; }
							return param;
						});

				try { return (*matching)(new_parameters, conversion); }
				catch (const exception::bad_boxed_cast&)
				{
					// parameter failed to cast
				}
				catch (const exception::arity_error&)
				{
					// invalid num params
				}
				catch (const exception::guard_error&)
				{
					// guard failed to allow the function to execute
				}

				throw exception::dispatch_error{
						params.to<parameters_type>(),
						const_function_proxies_type{
								functions.begin(),
								functions.end()}};
			}
		}// namespace proxy_function_detail
	}
}

#endif // GAL_LANG_FOUNDATION_FUNCTION_PROXY_HPP
