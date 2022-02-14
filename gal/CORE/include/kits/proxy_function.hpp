#pragma once

#ifndef GAL_LANG_KITS_PROXY_FUNCTION_HPP
#define GAL_LANG_KITS_PROXY_FUNCTION_HPP

#include <vector>
#include <utils/assert.hpp>
#include <defines.hpp>
#include <kits/function_parameters.hpp>
#include <kits/return_handler.hpp>
#include<kits/dynamic_object.hpp>
#include<kits/boxed_value_cast.hpp>

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
		std::vector<gal_type_info> build_params_type_list(Result (*)(Params ...)) { return {utils::make_type_info<Result>(), utils::make_type_info<Params>()...}; }

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
		using param_type_type = detail::gal_type_info;

		using param_type = std::pair<param_name_type, param_type_type>;
		using param_type_container_type = std::vector<param_type>;

	private:
		param_type_container_type types_;
		bool empty_;

		void check_empty()
		{
			for (const auto& [name, _]: types_)
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

			const auto dynamic_object_type_info = utils::make_type_info<dynamic_object>();

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
			const auto dynamic_object_type_info = utils::make_type_info<dynamic_object>();

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
								not(dynamic_object_name::match(name.c_str()) || result.type_name() == name)) { return std::make_pair(false, false); }
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
	class proxy_function_base
	{
	public:
		proxy_function_base() = default;

		proxy_function_base(const proxy_function_base&) = default;
		proxy_function_base& operator=(const proxy_function_base&) = default;
		proxy_function_base(proxy_function_base&&) = default;
		proxy_function_base& operator=(proxy_function_base&&) = default;

		virtual ~proxy_function_base() noexcept = default;

	protected:
		static_assert(std::is_signed_v<arity_error::size_type>);
		using arity_size_type = arity_error::size_type;

		std::vector<detail::gal_type_info> types_;
		arity_error::size_type arity_;
		bool has_arithmetic_param_;

		static bool compare_type(const std::vector<detail::gal_type_info>& tis, const function_parameters& params, const type_conversion_state& conversion) noexcept
		{
			if (tis.size() - 1 != params.size()) { return false; }

			for (decltype(params.size()) i = 0; i < params.size(); ++i) { if (not compare_type_to_param(tis[i + 1], params[i], conversion)) { return false; } }

			return true;
		}

		proxy_function_base(std::vector<detail::gal_type_info> types, const arity_size_type arity)
			: types_{std::move(types)},
			  arity_{arity},
			  has_arithmetic_param_{std::ranges::any_of(types, [](const auto& type) { return type.is_arithmetic(); })} { }

		[[nodiscard]] virtual boxed_value do_invoke(const function_parameters& params, const type_conversion_state& conversion) const = 0;

	public:
		static bool compare_type_to_param(const utils::gal_type_info& ti, const boxed_value& object, const type_conversion_state& conversion) noexcept
		{
			const auto boxed_value_type_info = utils::make_type_info<boxed_value>();
			const auto boxed_number_type_info = utils::make_type_info<boxed_number>();
			const auto function_type_info = utils::make_type_info<std::shared_ptr<const proxy_function_base>>();

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
		[[nodiscard]] const std::vector<detail::gal_type_info>& types() const noexcept { return types_; }

		[[nodiscard]] constexpr bool has_arithmetic_param() const noexcept { return has_arithmetic_param_; }

		/**
		 * @brief Return true if the function is a possible match to the passed in values.
		 */
		[[nodiscard]] bool filter(const function_parameters& params, const type_conversion_state& conversion) const noexcept
		{
			gal_assert(arity_ == -1 || (arity_ > 0 && static_cast<arity_size_type>(params.size()) == arity_));

			if (arity_ < 0) { return true; }

			bool result = compare_type_to_param(types_[1], params[0], conversion);

			if (arity_ > 1) { result &= compare_type_to_param(types_[2], params[1], conversion); }

			return result;
		}

		[[nodiscard]] constexpr virtual bool is_attribute_function() const noexcept { return false; }

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
		static std::vector<detail::gal_type_info> build_param_type_list(const param_types& types)
		{
			std::vector ret{detail::type_info_factory<boxed_value>::make()};

			for (const auto& [_, ti]: types.types())
			{
				if (ti.is_undefined()) { ret.push_back(detail::type_info_factory<boxed_value>::make()); }
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
				catch (const arity_error&) { return false; }catch (const bad_boxed_cast&) { return false; }
			}
			return true;
		}

		/**
		 * @return pair.first means 'is a match or not', pair.second means 'needs conversions'
		 */
		[[nodiscard]] std::pair<bool, bool> do_match(const function_parameters& params, const type_conversion_state& conversion) const
		{
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

	template<typename Callable>
	class dynamic_proxy_function final : public dynamic_proxy_function_base
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
}

#endif // GAL_LANG_KITS_PROXY_FUNCTION_HPP
