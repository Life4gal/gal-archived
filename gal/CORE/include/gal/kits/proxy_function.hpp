#pragma once

#ifndef GAL_LANG_KITS_PROXY_FUNCTION_HPP
#define GAL_LANG_KITS_PROXY_FUNCTION_HPP

#include <vector>
#include <utils/assert.hpp>
#include <utils/algorithm.hpp>
#include <gal/defines.hpp>
#include <ranges>
#include <gal/kits/function_parameters.hpp>
#include <gal/kits/return_handler.hpp>
#include <gal/kits/dynamic_object.hpp>
#include <gal/kits/boxed_value_cast.hpp>

namespace gal::lang
{
	struct ast_node;
	using ast_node_ptr = std::unique_ptr<ast_node>;
}

namespace gal::lang::kits
{
	namespace detail
	{
		/**
		 * @brief Used by proxy_function_impl to return a list of all param types it contains.
		 */
		template<typename Result, typename... Params>
		std::vector<utility::gal_type_info> build_params_type_list(Result (*)(Params ...)) { return {utility::make_type_info<Result>(), utility::make_type_info<Params>()...}; }

		/**
		 * @brief Used by proxy_function_impl to determine if it is equivalent to another
		 * proxy_function_impl object. This function is primarily used to prevent registration
		 * of two functions with the exact same signatures.
		 */
		template<typename Result, typename... Params>
		bool is_invokable(
				Result (*)(Params ...),
				const function_parameters& params,
				const type_conversion_state& conversion) noexcept
		{
			try
			{
				[&, i = function_parameters::size_type{0}]() mutable { (boxed_cast<Params>(params[i++], &conversion), ...); }();

				return true;
			}
			catch (const bad_boxed_cast&) { return false; }
		}

		template<typename Callable, typename Result, typename... Params, std::size_t... Index>
		Result do_invoke(
				Result (*)(Params ...),
				std::index_sequence<Index...>,
				const Callable& function,
				const function_parameters& params,
				const type_conversion_state& conversion
				) { return function(boxed_cast<Params>(params[Index], &conversion)...); }

		/**
		 * @brief Used by proxy_function_impl to perform type safe execution of a function.
		 * @note The function attempts to unbox each parameter to the expected type.
		 * if any unboxing fails the execution of the function fails and
		 * the bad_boxed_cast is passed up to the caller.
		 */
		template<typename Callable, typename Result, typename... Params>
		boxed_value do_invoke(
				Result (*signature)(Params ...),
				const Callable& function,
				const function_parameters& params,
				const type_conversion_state& conversion
				)
		{
			if constexpr (std::is_same_v<Result, void>)
			{
				do_invoke(signature, std::index_sequence_for<Params...>{}, function, params, conversion);
				return return_handler<void>::handle();
			}
			else { return return_handler<Result>::handle(do_invoke(signature, std::index_sequence_for<Params...>{}, function, params, conversion)); }
		}
	}

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

	template<typename Function>
	std::function<Function> make_functor(std::shared_ptr<const proxy_function_base> function, const type_conversion_state* conversion);

	class param_types
	{
	public:
		using param_name_type = std::string;
		using param_type_type = utility::gal_type_info;

		using param_type = std::pair<param_name_type, param_type_type>;
		using param_type_container_type = std::vector<param_type>;

	private:
		param_type_container_type types_;
		bool empty_;

		void check_empty()
		{
			for (const auto& name: types_ | std::views::keys)
			{
				if (not name.empty())
				{
					empty_ = false;
					return;
				}
			}
		}

	public:
		param_types()
			: empty_{true} {}

		explicit param_types(param_type_container_type types)
			: types_{std::move(types)},
			  empty_{true} { check_empty(); }

		bool operator==(const param_types& other) const noexcept { return types_ == other.types_; }

		void push_front(param_name_type name, param_type_type ti)
		{
			types_.emplace(types_.begin(), std::move(name), ti);
			check_empty();
		}

		[[nodiscard]] std::vector<boxed_value> convert(const function_parameters& params, const type_conversion_state& conversion) const
		{
			auto ret = params.to<std::vector>();

			const auto dynamic_object_type_info = utility::make_type_info<dynamic_object>();

			for (decltype(ret.size()) i = 0; i < ret.size(); ++i)
			{
				if (const auto& [name, ti] = types_[i];
					not name.empty())
				{
					if (const auto& object = ret[i];
						not object.type_info().bare_equal(dynamic_object_type_info))
					{
						if (not ti.is_undefined())
						{
							if (not object.type_info().bare_equal(ti))
							{
								if (conversion->is_convertible_type(ti, object.type_info()))
								{
									try
									{
										// We will not catch any bad_boxed_dynamic_cast that is thrown, let the user get it
										// either way, we are not responsible if it doesn't work
										ret[i] = conversion->boxed_type_conversion(ti, conversion.saves(), ret[i]);
									}
									catch (...)
									{
										try
										{
											// try going the other way
											ret[i] = conversion->boxed_type_down_conversion(ti, conversion.saves(), ret[i]);
										}
										catch (const std::bad_any_cast&) { throw bad_boxed_cast{object.type_info(), ti.bare_type_info()}; }
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
		 * @return pair.first means 'is a match or not', pair.second means 'needs conversions'
		 */
		[[nodiscard]] std::pair<bool, bool> match(const function_parameters& params, const type_conversion_state& conversion) const noexcept
		{
			const auto dynamic_object_type_info = utility::make_type_info<dynamic_object>();

			bool need_conversion = false;

			if (empty_) { return std::make_pair(true, need_conversion); }
			if (params.size() != types_.size()) { return std::make_pair(false, need_conversion); }

			for (decltype(params.size()) i = 0; i < params.size(); ++i)
			{
				const auto& [name, ti] = types_[i];

				if (not name.empty())
				{
					if (const auto& object = params[i];
						object.type_info().bare_equal(dynamic_object_type_info))
					{
						try
						{
							if (const auto& result = boxed_cast<const dynamic_object&>(object, &conversion);
								not(dynamic_object_name::match(name) || result.type_name() == name)) { return std::make_pair(false, false); }
						}
						catch (const std::bad_cast&) { return std::make_pair(false, false); }
					}
					else
					{
						if (not ti.is_undefined())
						{
							if (not object.type_info().bare_equal(ti))
							{
								if (not conversion->is_convertible_type(ti, object.type_info())) { return std::make_pair(false, false); }
								need_conversion = true;
							}
						}
						else { return std::make_pair(false, false); }
					}
				}
			}

			return std::make_pair(true, need_conversion);
		}

		[[nodiscard]] const param_type_container_type& types() const noexcept { return types_; }
	};

	/**
	 * @brief Pure virtual base class for all proxy_function implementations.
	 * proxy_functions are a type erasure of type safe C++ function calls.
	 * At runtime parameter types are expected to be tested against passed in types.
	 * dispatch_engine only knows how to work with proxy_function, no other
	 * function classes.
	 */
	class proxy_function_base// NOLINT(cppcoreguidelines-pro-type-member-init)
	{
	public:
		proxy_function_base() = default;

		proxy_function_base(const proxy_function_base&) = default;
		proxy_function_base& operator=(const proxy_function_base&) = default;
		proxy_function_base(proxy_function_base&&) = default;
		proxy_function_base& operator=(proxy_function_base&&) = default;

		virtual ~proxy_function_base() noexcept = default;

		static_assert(std::is_signed_v<arity_error::size_type>);
		using arity_size_type = arity_error::size_type;
		constexpr static arity_size_type no_parameters_arity = -1;

		using type_infos_type = std::vector<utility::gal_type_info>;
		using arguments_type = std::vector<boxed_value>;
		using contained_functions_type = std::vector<std::shared_ptr<const proxy_function_base>>;

	protected:
		type_infos_type types_;
		arity_error::size_type arity_;
		bool has_arithmetic_param_;

		static bool compare_type(const type_infos_type& tis, const function_parameters& params, const type_conversion_state& conversion) noexcept
		{
			if (tis.size() - 1 != params.size()) { return false; }

			for (decltype(params.size()) i = 0; i < params.size(); ++i) { if (not compare_type_to_param(tis[i + 1], params[i], conversion)) { return false; } }

			return true;
		}

		proxy_function_base(type_infos_type types, const arity_size_type arity)
			: types_{std::move(types)},
			  arity_{arity},
			  has_arithmetic_param_{std::ranges::any_of(types_, [](const auto& type) { return type.is_arithmetic(); })} { }

		[[nodiscard]] virtual boxed_value do_invoke(const function_parameters& params, const type_conversion_state& conversion) const = 0;

	public:
		static bool compare_type_to_param(const utility::gal_type_info& ti, const boxed_value& object, const type_conversion_state& conversion) noexcept
		{
			const auto boxed_value_type_info = utility::make_type_info<boxed_value>();
			const auto boxed_number_type_info = utility::make_type_info<boxed_number>();
			const auto function_type_info = utility::make_type_info<std::shared_ptr<const proxy_function_base>>();

			if (
				ti.is_undefined() ||
				ti.bare_equal(boxed_value_type_info) ||
				(
					not object.type_info().is_undefined() &&
					(
						(ti.bare_equal(boxed_number_type_info) && object.type_info().is_arithmetic()) ||
						ti.bare_equal(object.type_info()) ||
						object.type_info().bare_equal(function_type_info) ||
						conversion->is_convertible_type(ti, object.type_info())
					)
				)
			) { return true; }
			return false;
		}

		boxed_value operator()(const function_parameters& params, const type_conversion_state& conversion) const
		{
			if (arity_ < 0 || static_cast<decltype(params.size())>(arity_) == params.size()) { return do_invoke(params, conversion); }
			throw arity_error{static_cast<arity_size_type>(params.size()), arity_};
		}

		/**
		 * @brief The number of arguments the function takes or -1 if it is variadic.
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
		[[nodiscard]] bool filter(const function_parameters& params, const type_conversion_state& conversion) const noexcept
		{
			gal_assert(arity_ == no_parameters_arity || (arity_ > 0 && static_cast<arity_size_type>(params.size()) == arity_));

			if (arity_ < 0) { return true; }

			bool result = compare_type_to_param(types_[1], params[0], conversion);

			if (arity_ > 1) { result &= compare_type_to_param(types_[2], params[1], conversion); }

			return result;
		}

		[[nodiscard]] constexpr virtual bool is_attribute_function() const noexcept { return false; }

		/**
		 * @return const_proxy_function vector
		 */
		[[nodiscard]] virtual contained_functions_type get_contained_function() const { return {}; }

		[[nodiscard]] virtual bool operator==(const proxy_function_base& other) const noexcept = 0;
		[[nodiscard]] virtual bool match(const function_parameters& params, const type_conversion_state& conversion) const = 0;

		[[nodiscard]] virtual bool is_first_type_match(const boxed_value& object, const type_conversion_state& conversion) const noexcept
		{
			gal_assert(types_.size() >= 2);
			return compare_type_to_param(types_[1], object, conversion);
		}
	};
}

namespace gal::lang
{
	/**
	 * @brief Common typedef used for passing of any registered function in GAL.
	 */
	using proxy_function = std::shared_ptr<kits::proxy_function_base>;

	/**
	 * @brief Const version of proxy_function. Points to a const proxy_function.
	 * This is how most registered functions are handled internally.
	 */
	using const_proxy_function = std::shared_ptr<const kits::proxy_function_base>;
}

namespace gal::lang::kits
{
	/**
	 * @brief Exception thrown if a function's guard fails.
	 */
	class guard_error final : public std::runtime_error
	{
	public:
		guard_error()
			: std::runtime_error{"Guard evaluation failed"} {}
	};

	namespace base
	{
		/**
	 * @brief A proxy_function implementation that is not type safe, the called
	 * function is expecting a std::vector<boxed_value> that it works with how it chooses.
	 */
		class dynamic_proxy_function_base : public proxy_function_base
		{
		public:
			using parse_ast_node_type = std::shared_ptr<ast_node>;

		private:
			parse_ast_node_type parse_ast_node_;
			proxy_function guard_;

		protected:
			param_types param_types_;

		private:
			static type_infos_type build_param_type_list(const param_types& types)
			{
				std::vector ret{utility::detail::type_info_factory<boxed_value>::make()};

				for (const auto& ti: types.types() | std::views::values)
				{
					if (ti.is_undefined()) { ret.push_back(utility::detail::type_info_factory<boxed_value>::make()); }
					else { ret.push_back(ti); }
				}

				return ret;
			}

		protected:
			[[nodiscard]] bool test_guard(const function_parameters& params, const type_conversion_state& conversion) const
			{
				if (guard_)
				{
					try { return boxed_cast<bool>(guard_->operator()(params, conversion)); }
					catch (const arity_error&) { return false; }
					catch (const bad_boxed_cast&) { return false; }
				}
				return true;
			}

			/**
		 * @return pair.first means 'is a match or not', pair.second means 'needs conversions'
		 */
			[[nodiscard]] std::pair<bool, bool> do_match(const function_parameters& params, const type_conversion_state& conversion) const
			{
				// ReSharper disable once CppVariableCanBeMadeConstexpr
				const auto [m, c] = [&]
				{
					if (arity_ < 0) { return std::make_pair(true, false); }
					if (static_cast<decltype(params.size())>(arity_) == params.size()) { return param_types_.match(params, conversion); }
					return std::make_pair(false, false);
				}();

				return std::make_pair(m && test_guard(params, conversion), c);
			}

		public:
			dynamic_proxy_function_base(
					const arity_size_type arity,
					parse_ast_node_type parse_ast_node,
					param_types param_types = {},
					proxy_function guard = {})
				: proxy_function_base{build_param_type_list(param_types), arity},
				  parse_ast_node_{std::move(parse_ast_node)},
				  guard_{std::move(guard)},
				  param_types_{std::move(param_types)} {}

			[[nodiscard]] const parse_ast_node_type::element_type& get_parse_tree() const
			{
				if (parse_ast_node_) { return *parse_ast_node_; }
				throw std::runtime_error{"Dynamic_proxy_function does not contain a parse_tree"};
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
					              param_types_ == rhs->param_types_;
				       }();
			}

			[[nodiscard]] bool match(const function_parameters& params, const type_conversion_state& conversion) const override { return do_match(params, conversion).first; }
		};
	}

	template<typename Callable>
	class dynamic_proxy_function final : public base::dynamic_proxy_function_base
	{
	private:
		Callable function_;

	protected:
		[[nodiscard]] boxed_value do_invoke(const function_parameters& params, const type_conversion_state& conversion) const override
		{
			if (const auto [m, c] = do_match(params, conversion);
				m)
			{
				if (c) { return function_(function_parameters{param_types_.convert(params, conversion)}); }
				return function_(params);
			}
			throw guard_error{};
		}

	public:
		dynamic_proxy_function(
				Callable function,
				const arity_size_type arity,
				parse_ast_node_type parse_ast_node,
				param_types types,
				proxy_function guard)
			: dynamic_proxy_function_base{arity, std::move(parse_ast_node), std::move(types), std::move(guard)},
			  function_{std::move(function)} {}
	};

	template<typename Callable, typename... Args>
	proxy_function make_dynamic_proxy_function(Callable&& function, Args&&... args)
	{
		return std::make_shared<proxy_function_base, dynamic_proxy_function<Callable>>(
				std::forward<Callable>(function),
				std::forward<Args>(args)...);
	}

	/**
	 * @brief An object used by bound_function to represent "_" parameters
	 * of a binding. This allows for unbound parameters during bind.
	 */
	struct function_argument_placeholder {};

	/**
	 * @brief An implementation of proxy_function that takes a proxy_function
	 * and substitutes bound parameters into the parameter list
	 *  at runtime, when call() is executed.
	 *
	 *	@note it is used for bind(function, param1, _, param2) style calls
	 */
	class bound_function final : public proxy_function_base
	{
	public:
		using arguments_type = std::vector<boxed_value>;

	private:
		const_proxy_function function_;
		arguments_type arguments_;

		static type_infos_type build_param_type_info(const const_proxy_function& function, const arguments_type& arguments)
		{
			gal_assert(function->get_arity() < 0 || function->get_arity() == static_cast<arity_size_type>(arguments.size()));

			if (function->get_arity() < 0) { return {}; }

			const auto& types = function->types();
			gal_assert(types.size() == arguments.size() + 1);

			std::vector ret{types[0]};

			for (decltype(arguments.size()) i = 0; i < arguments.size(); ++i) { if (arguments[i].type_info() == utility::detail::type_info_factory<function_argument_placeholder>::make()) { ret.push_back(types[i + 1]); } }

			return ret;
		}

		[[nodiscard]] boxed_value do_invoke(const function_parameters& params, const type_conversion_state& conversion) const override { return (*function_)(function_parameters{build_parameters_list(params)}, conversion); }

	public:
		bound_function(const const_proxy_function& function, const arguments_type& arguments)
			: proxy_function_base{build_param_type_info(function, arguments), function->get_arity() < 0 ? static_cast<arity_size_type>(-1) : static_cast<arity_size_type>(build_param_type_info(function, arguments).size()) - 1},
			  function_{function},
			  arguments_{arguments} { gal_assert(function->get_arity() < 0 || function->get_arity() == static_cast<arity_size_type>(arguments.size())); }

		[[nodiscard]] arguments_type build_parameters_list(const function_parameters& params) const
		{
			return [
						it_param = params.begin(),
						it_arg = arguments_.begin(),

						end_param = params.end(),
						end_arg = arguments_.end()]() mutable
					{
						arguments_type ret{};

						while (not (it_param == end_param && it_arg == end_arg))
						{
							while (it_arg != end_arg && it_arg->type_info() != utility::detail::type_info_factory<function_argument_placeholder>::make())
							{
								ret.push_back(*it_arg);
								++it_arg;
							}

							if (it_param != end_param)
							{
								ret.push_back(*it_param);
								++it_param;
							}

							if (it_arg != end_arg && it_arg->type_info() == utility::detail::type_info_factory<function_argument_placeholder>::make()) { ++it_arg; }
						}

						return ret;
					}();
		}

		[[nodiscard]] contained_functions_type get_contained_function() const override { return {function_}; }

		[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override { return &other == this; }

		[[nodiscard]] bool match(const function_parameters& params, const type_conversion_state& conversion) const override { return function_->match(function_parameters{build_parameters_list(params)}, conversion); }
	};

	namespace base
	{
		class proxy_function_impl_base : public proxy_function_base
		{
		public:
			explicit proxy_function_impl_base(const type_infos_type& types)
				: proxy_function_base{types, static_cast<arity_size_type>(types.size()) - 1} {}

			[[nodiscard]] bool match(const function_parameters& params, const type_conversion_state& conversion) const override
			{
				return static_cast<arity_size_type>(params.size()) == get_arity() &&
				       (compare_type(types_, params, conversion) && is_invokable(params, conversion));
			}

			[[nodiscard]] virtual bool is_invokable(const function_parameters& params, const type_conversion_state& conversion) const noexcept = 0;
		};
	}

	/**
	 * @brief For any callable object
	 */
	template<typename FunctionSignature, typename Callable>
	class proxy_function_callable final : public base::proxy_function_impl_base
	{
	private:
		Callable function_;

	protected:
		[[nodiscard]] boxed_value do_invoke(const function_parameters& params, const type_conversion_state& conversion) const override { return detail::do_invoke(static_cast<FunctionSignature*>(nullptr), function_, params, conversion); }

	public:
		explicit proxy_function_callable(Callable function)
			: proxy_function_impl_base{detail::build_params_type_list(static_cast<FunctionSignature*>(nullptr))},
			  function_{std::move(function)} {}

		[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override { return dynamic_cast<const proxy_function_callable<FunctionSignature, Callable>*>(&function_); }

		[[nodiscard]] bool is_invokable(const function_parameters& params, const type_conversion_state& conversion) const noexcept override { return detail::is_invokable(static_cast<FunctionSignature*>(nullptr), params, conversion); }
	};

	namespace base
	{
		class assignable_proxy_function_base : public proxy_function_impl_base
		{
		public:
			explicit assignable_proxy_function_base(const type_infos_type& types)
				: proxy_function_impl_base{types} {}

			virtual void assign(const const_proxy_function& other) = 0;
		};
	}

	template<typename FunctionSignature>
	class assignable_proxy_function final : public base::assignable_proxy_function_base
	{
	private:
		std::reference_wrapper<std::function<FunctionSignature>> function_;
		std::shared_ptr<std::function<FunctionSignature>> shared_function_;

	protected:
		[[nodiscard]] boxed_value do_invoke(const function_parameters& params, const type_conversion_state& conversion) const override { return detail::do_invoke(static_cast<FunctionSignature*>(nullptr), function_.get(), params, conversion); }

	public:
		assignable_proxy_function(
				std::reference_wrapper<std::function<FunctionSignature>> function,
				std::shared_ptr<std::function<FunctionSignature>> shared_function
				)
			: assignable_proxy_function_base{detail::build_params_type_list(static_cast<FunctionSignature*>(nullptr))},
			  function_{std::move(function)},
			  shared_function_{std::move(shared_function)} { gal_assert(not shared_function_ || shared_function_.get() == &function_.get()); }

		[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override { return dynamic_cast<const assignable_proxy_function<FunctionSignature>*>(&other); }

		[[nodiscard]] bool is_invokable(const function_parameters& params, const type_conversion_state& conversion) const noexcept override { return detail::is_invokable(static_cast<FunctionSignature*>(nullptr), params, conversion); }

		void assign(const const_proxy_function& other) override { function_.get() = make_functor<FunctionSignature>(other, nullptr); }
	};

	template<typename T, typename Class>
	class attribute_accessor final : public proxy_function_base
	{
	public:
		constexpr static arity_size_type arity_size = 1;

	private:
		type_infos_type param_types_;
		T Class::* attribute_;

		static type_infos_type param_types() { return {utility::make_type_info<T>(), utility::make_type_info<Class>()}; }

		template<typename U>
		auto do_invoke(Class* object) const
		{
			if constexpr (std::is_pointer_v<U>) { return detail::return_handler<U>::handle(object->*attribute_); }
			else { return detail::return_handler<std::add_lvalue_reference_t<U>>::handle(object->*attribute_); }
		}

		template<typename U>
		auto do_invoke(const Class* object) const
		{
			if constexpr (std::is_pointer_v<U>) { return detail::return_handler<const U>::handle(object->*attribute_); }
			else { return detail::return_handler<std::add_lvalue_reference_t<std::add_const_t<U>>>::handle(object->*attribute_); }
		}

	protected:
		[[nodiscard]] boxed_value do_invoke(const function_parameters& params, const type_conversion_state& conversion) const override
		{
			const auto& object = params.front();

			if (object.is_const()) { return do_invoke<T>(boxed_cast<const Class*>(object, &conversion)); }
			return do_invoke<T>(boxed_cast<Class*>(object, &conversion));
		}

	public:
		explicit attribute_accessor(T Class::* attribute)
			: proxy_function_base{param_types(), arity_size},
			  param_types_{std::move(param_types())},
			  attribute_{attribute} {}

		[[nodiscard]] constexpr bool is_attribute_function() const noexcept override { return true; }

		[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override
		{
			if (
				const auto* accessor = dynamic_cast<const attribute_accessor<T, Class>*>(&other);
				accessor
			) { return attribute_ == accessor->attribute_; }
			return false;
		}

		[[nodiscard]] bool match(const function_parameters& params, const type_conversion_state& conversion) const override
		{
			if (static_cast<arity_size_type>(params.size()) != arity_size) { return false; }

			return params.front().type_info().bare_equal(utility::make_type_info<Class>());
		}
	};

	class dispatch_error final : public std::runtime_error
	{
	public:
		proxy_function_base::arguments_type parameters;
		proxy_function_base::contained_functions_type functions;

		dispatch_error(
				const function_parameters& parameters,
				proxy_function_base::contained_functions_type functions,
				const std::string_view message = "Error with function dispatch"
				)
			: std::runtime_error{message.data()},
			  parameters{parameters.to<proxy_function_base::arguments_type>()},
			  functions{std::move(functions)} {}
	};

	namespace detail
	{
		template<typename Function>
		bool types_match_except_for_arithmetic(
				const Function& function,
				const function_parameters& parameters,
				const type_conversion_state& conversion
				) noexcept
		{
			const auto& types = function->types();

			if (function->get_arity() == proxy_function_base::no_parameters_arity) { return false; }

			gal_assert(parameters.size() == types.size() - 1);

			return std::mismatch(
					       parameters.begin(),
					       parameters.end(),
					       types.begin() + 1,
					       [&](const auto& object, const auto& ti) { return proxy_function_base::compare_type_to_param(ti, object, conversion) || (object.type_info().is_arithmetic() && ti.is_arithmetic()); }) == std::make_pair(parameters.end(), types.end());
		}

		template<typename Iterator, typename Functions>
		boxed_value dispatch_with_conversion(
				Iterator begin,
				const Iterator end,
				const function_parameters& parameters,
				const type_conversion_state& conversion,
				const Functions& functions
				)
		{
			Iterator matching{end};

			while (begin != end)
			{
				if (types_match_except_for_arithmetic(begin->second, parameters, conversion))
				{
					if (matching == end) { matching = begin; }
					else
					{
						// handle const members vs non-const member, which is not really ambiguous
						const auto& match_function_param_types = matching->second->types();
						const auto& next_function_param_types = begin->second->types();

						if (
							parameters.front().is_const() &&
							not match_function_param_types[1].is_const() &&
							next_function_param_types[1].is_const()
						)
						{
							// keep the new one, the const/non-const match up is correct
							matching = begin;
						}
						else if (
							not parameters.front().is_const() &&
							not match_function_param_types[1].is_const() &&
							next_function_param_types[1].is_const()
						)
						{
							// keep the old one, it has a better const/non-const match up
							// do nothing
						}
						else
						{
							// ambiguous function call
							throw dispatch_error{
									parameters,
									proxy_function_base::contained_functions_type{
											functions.begin(), functions.end()
									}};
						}
					}
				}

				++begin;
			}

			if (matching == end)
			{
				// no appropriate function to attempt arithmetic type conversion on
				throw dispatch_error{
						parameters,
						proxy_function_base::contained_functions_type{
								functions.begin(),
								functions.end()}};
			}

			std::vector<boxed_value> new_parameters;
			new_parameters.reserve(parameters.size());

			const auto& tis = matching->second->types();

			std::transform(
					tis.begin() + 1,
					tis.end(),
					parameters.begin(),
					std::back_inserter(new_parameters),
					[](const auto& ti, const auto& param) -> boxed_value
					{
						if (ti.is_arithmetic() && param.type_info().is_arithmetic() && param.type_info() != ti) { return boxed_number{param}.as(ti).value; }
						return param;
					});

			try { return (*(matching->second))(function_parameters{new_parameters}, conversion); }
			catch (const bad_boxed_cast&)
			{
				// parameter failed to cast
			}
			catch (const arity_error&)
			{
				// invalid num params
			}
			catch (const guard_error&)
			{
				// guard failed to allow the function to execute
			}

			throw dispatch_error{
					parameters,
					proxy_function_base::contained_functions_type{
							functions.begin(),
							functions.end()}};
		}
	}

	template<typename Functions>
	boxed_value dispatch(
			const Functions& functions,
			const function_parameters& parameters,
			const type_conversion_state& conversion)
	{
		std::vector<std::pair<std::size_t, const proxy_function_base*>> ordered_functions;
		ordered_functions.reserve(functions.size());

		for (const auto& function: functions)
		{
			const auto arity = function->get_arity();

			if (arity == proxy_function_base::no_parameters_arity) { ordered_functions.emplace_back(parameters.size(), function.get()); }
			else if (arity == static_cast<proxy_function_base::arity_size_type>(parameters.size()))
			{
				std::size_t num_diffs = 0;

				utils::zip_invoke(
						[&num_diffs](const auto& ti, const auto& param) { if (not ti.bare_equal(param.type_info())) { ++num_diffs; } },
						function->types().begin() + 1,
						function->types().end(),
						parameters.begin());

				ordered_functions.emplace_back(num_diffs, function.get());
			}
		}

		for (decltype(parameters.size()) i = 0; i < parameters.size() + 1; ++i)
		{
			for (const auto& [order, function]: ordered_functions)
			{
				try { if (order == i && (i == 0 || function->filter(parameters, conversion))) { return (*(function))(parameters, conversion); } }
				catch (const bad_boxed_cast&)
				{
					// parameter failed to cast, try again
				}
				catch (const arity_error&)
				{
					// invalid num params, try again
				}
				catch (const guard_error&)
				{
					// guard failed to allow the function to execute, try again
				}
			}
		}

		return detail::dispatch_with_conversion(ordered_functions.begin(), ordered_functions.end(), parameters, conversion, functions);
	}
}

#endif // GAL_LANG_KITS_PROXY_FUNCTION_HPP
