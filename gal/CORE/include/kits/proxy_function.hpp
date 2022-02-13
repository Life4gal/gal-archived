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
				[i = function_parameters::size_type{0}, &]() mutable { (boxed_cast<Params>(params[i++], &conversion), ...); }();

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

	class params_type
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
		params_type()
			: empty_{true} {}

		explicit params_type(param_type_container_type types)
			: types_{std::move(types)},
			  empty_{true} { check_empty(); }

		bool operator==(const params_type& other) const noexcept { return types_ == other.types_; }

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

		std::vector<detail::gal_type_info> types_;
		arity_error::size_type arity_;
		bool has_arithmetic_param_;

		static bool compare_type(const std::vector<detail::gal_type_info>& tis, const function_parameters& params, const type_conversion_state& conversion) noexcept
		{
			if (tis.size() - 1 != params.size()) { return false; }

			for (decltype(params.size()) i = 0; i < params.size(); ++i) { if (not compare_type_to_param(tis[i + 1], params[i], conversion)) { return false; } }

			return true;
		}

		proxy_function_base(std::vector<detail::gal_type_info> types, const arity_error::size_type arity)
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
			throw arity_error{static_cast<arity_error::size_type>(params.size()), arity_};
		}

		/**
		 * @brief The number of arguments the function takes or -1 if it is variadic.
		 */
		[[nodiscard]] constexpr auto get_arity() const noexcept { return arity_; }

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
			gal_assert(arity_ == -1 || (arity_ > 0 && static_cast<arity_error::size_type>(params.size()) == arity_));

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

#endif // GAL_LANG_KITS_PROXY_FUNCTION_HPP
