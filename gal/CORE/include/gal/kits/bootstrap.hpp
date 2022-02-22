#pragma once

#ifndef GAL_LANG_KITS_BOOTSTRAP_HPP
#define GAL_LANG_KITS_BOOTSTRAP_HPP

#include <utils/format.hpp>
#include <gal/kits/register_function.hpp>
#include <gal/kits/dispatch.hpp>
#include <gal/kits/operators.hpp>
#include <gal/kits/proxy_constructor.hpp>

namespace gal::lang::kits
{
	template<typename T>
		requires std::is_array_v<T>
	void register_array_type(const std::string_view name, engine_module& m)
	{
		using return_type = std::remove_extent_t<T>;

		m.add_type_info(name, utility::make_type_info<T>());

		m.add_function(
				"[]",
				fun([](T& arr, std::size_t index) -> return_type&
				{
					constexpr auto extent = std::extent_v<T>;
					if (index >= extent)
					{
						throw std::range_error{
								std_format::format(
										"Array index out of range. Array size if {} but received index is{}.",
										extent,
										index)};
					}
					return arr[index];
				}));
		m.add_function(
				"[]",
				fun([](const T& arr, std::size_t index) -> const return_type&
				{
					constexpr auto extent = std::extent_v<T>;
					if (index >= extent)
					{
						throw std::range_error{
								std_format::format(
										"Array index out of range. Array size if {} but received index is{}.",
										extent,
										index)};
					}
					return arr[index];
				}));

		m.add_function(
				"size",
				fun([](const auto&) { return std::extent_v<T>; }));

		// todo: maybe more interface?
	}

	/**
	 * @brief Add all comparison operators for the templated type.
	 * Used during bootstrap, also available to users.
	 *
	 * @tparam T Type to create comparison operators for.
	 */
	template<typename T>
	void register_comparison(engine_module& m)
	{
		detail::register_equal<T>(m);
		detail::register_not_equal<T>(m);
		detail::register_less_than<T>(m);
		detail::register_less_equal<T>(m);
		detail::register_greater_than<T>(m);
		detail::register_greater_equal<T>(m);
	}

	/**
	 * @brief Class consisting of only static functions.
	 * @note All default bootstrapping occurs from this class.
	 */
	class bootstrap
	{
	private:
		/**
		 * @brief Function allowing for assignment of an unknown type to any other value.
		 */
		static boxed_value unknown_assign(boxed_value lhs, const boxed_value& rhs)
		{
			if (lhs.is_undefined()) { return lhs.assign(rhs); }
			throw bad_boxed_cast{"boxed_value has a set type already"};
		}

		/**
		 * @brief Add all arithmetic operators for PODs
		 */
		static void register_all_arithmetic_operators(engine_module& m)
		{
			m.add_function(
					operator_assign_name::value,
					fun(&boxed_number::operator_assign));
			m.add_function(
					operator_equal_name::value,
					fun(&boxed_number::operator_equal));
			m.add_function(
					operator_not_equal_name::value,
					fun(&boxed_number::operator_not_equal));
			m.add_function(
					operator_less_than_name::value,
					fun(&boxed_number::operator_less_than));
			m.add_function(
					operator_less_equal_name::value,
					fun(&boxed_number::operator_less_equal));
			m.add_function(
					operator_greater_than_name::value,
					fun(&boxed_number::operator_greater_than));
			m.add_function(
					operator_greater_equal_name::value,
					fun(&boxed_number::operator_greater_equal));
			m.add_function(
					operator_plus_name::value,
					fun(&boxed_number::operator_plus));
			m.add_function(
					operator_minus_name::value,
					fun(&boxed_number::operator_minus));
			m.add_function(
					operator_multiply_name::value,
					fun(&boxed_number::operator_multiply));
			m.add_function(
					operator_divide_name::value,
					fun(&boxed_number::operator_divide));
			m.add_function(
					operator_remainder_name::value,
					fun(&boxed_number::operator_remainder));
			m.add_function(
					operator_plus_assign_name::value,
					fun(&boxed_number::operator_plus_assign));
			m.add_function(
					operator_minus_assign_name::value,
					fun(&boxed_number::operator_minus_assign));
			m.add_function(
					operator_multiply_assign_name::value,
					fun(&boxed_number::operator_multiply_assign));
			m.add_function(
					operator_divide_assign_name::value,
					fun(&boxed_number::operator_divide_assign));
			m.add_function(
					operator_remainder_assign_name::value,
					fun(&boxed_number::operator_remainder_assign));
			m.add_function(
					operator_bitwise_shift_left_name::value,
					fun(&boxed_number::operator_bitwise_shift_left));
			m.add_function(
					operator_bitwise_shift_right_name::value,
					fun(&boxed_number::operator_bitwise_shift_right));
			m.add_function(
					operator_bitwise_and_name::value,
					fun(&boxed_number::operator_bitwise_and));
			m.add_function(
					operator_bitwise_or_name::value,
					fun(&boxed_number::operator_bitwise_or));
			m.add_function(
					operator_bitwise_xor_name::value,
					fun(&boxed_number::operator_bitwise_xor));
			m.add_function(
					operator_bitwise_shift_left_assign_name::value,
					fun(&boxed_number::operator_bitwise_shift_left_assign));
			m.add_function(
					operator_bitwise_shift_right_assign_name::value,
					fun(&boxed_number::operator_bitwise_shift_right_assign));
			m.add_function(
					operator_bitwise_and_assign_name::value,
					fun(&boxed_number::operator_bitwise_and_assign));
			m.add_function(
					operator_bitwise_or_assign_name::value,
					fun(&boxed_number::operator_bitwise_or_assign));
			m.add_function(
					operator_bitwise_xor_assign_name::value,
					fun(&boxed_number::operator_bitwise_xor_assign));
			m.add_function(
					operator_unary_not_name::value,
					fun(&boxed_number::operator_unary_not));
			m.add_function(
					operator_unary_plus_name::value,
					fun(&boxed_number::operator_unary_plus));
			m.add_function(
					operator_unary_minus_name::value,
					fun(&boxed_number::operator_unary_minus));
			m.add_function(
					operator_unary_bitwise_complement_name::value,
					fun(&boxed_number::operator_unary_bitwise_complement));
		}

		/**
		 * @brief Create a bound function object. The first param is the function to bind
		 * the remaining parameters are the args to bind into the result.
		 */
		static boxed_value bind_function(const function_parameters& params)
		{
			if (params.empty()) { throw arity_error{1, 0}; }

			auto function = boxed_cast<const_proxy_function>(params.front());

			if (const auto arity = function->get_arity();
				arity != proxy_function_base::no_parameters_arity &&
				arity != static_cast<proxy_function_base::arity_size_type>(params.size() - 1)) { throw arity_error{arity, static_cast<proxy_function_base::arity_size_type>(params.size() - 1)}; }

			return boxed_value{const_proxy_function{std::make_shared<bound_function>(std::move(function), bound_function::arguments_type{params.begin() + 1, params.end()})}};
		}

		static bool has_guard(const const_proxy_function& function) noexcept
		{
			const auto f = std::dynamic_pointer_cast<const base::dynamic_proxy_function_base>(function);
			return f && f->has_guard();
		}

		static const_proxy_function get_guard(const const_proxy_function& function)
		{
			if (const auto f = std::dynamic_pointer_cast<const base::dynamic_proxy_function_base>(function);
				f && f->has_guard()) { return f->get_guard(); }
			throw std::runtime_error{"Function does not have a guard"};
		}

		template<typename FunctionType>
		static auto make_do_invoke(const FunctionType& function)
		{
			return [function](const proxy_function_base* base) -> std::vector<boxed_value>
			{
				const auto& result = (base->*function)();

				std::vector<boxed_value> ret{};
				ret.reserve(result.size());

				std::ranges::for_each(
						result,
						[&ret](const auto& object) { ret.push_back(const_var(object)); });

				return ret;
			};
		}

		static bool has_parse_tree(const const_proxy_function& function) noexcept { return std::dynamic_pointer_cast<const base::dynamic_proxy_function_base>(function).operator bool(); }

		static const ast_node& get_parse_tree(const const_proxy_function& function)
		{
			if (const auto f = std::dynamic_pointer_cast<const base::dynamic_proxy_function_base>(function);
				f) { return f->get_parse_tree(); }
			throw std::runtime_error{"Function does not have a parse tree"};
		}

	public:
		/**
		 * @brief perform all common bootstrap functions for std::string, void and POD types.
		 */
		static void do_bootstrap(engine_module& m)
		{
			//*********************************************
			// builtin type
			//*********************************************
			m.add_type_info(void_type_name::value,
			                utility::make_type_info<void>());
			m.add_type_info(boolean_type_name::value,
			                utility::make_type_info<bool>());
			m.add_type_info(object_type_name::value,
			                utility::make_type_info<boxed_value>());
			m.add_type_info(number_type_name::value,
			                utility::make_type_info<boxed_number>());

			//*********************************************
			// function & interface
			//*********************************************
			m.add_type_info(function_type_name::value,
			                utility::make_type_info<proxy_function>());
			m.add_type_info(assignable_function_type_name::value,
			                utility::make_type_info<base::assignable_proxy_function_base>());

			m.add_function(function_get_arity_interface_name::value,
			               fun(&proxy_function_base::get_arity));
			m.add_function(function_equal_interface_name::value,
			               fun(&proxy_function_base::operator==));
			m.add_function(function_get_param_types_interface_name::value,
			               fun(make_do_invoke(&proxy_function_base::types)));
			m.add_function(function_get_contained_functions_interface_name::value,
			               fun(make_do_invoke(&proxy_function_base::get_contained_function)));

			//*********************************************
			// dynamic object & interface
			//*********************************************
			m.add_type_info(dynamic_object_type_name::value,
			                utility::make_type_info<dynamic_object>());

			m.add_function(dynamic_object_type_name::value,
			               make_constructor<dynamic_object(const dynamic_object::type_name_type&)>());
			m.add_function(dynamic_object_type_name::value,
			               make_constructor<dynamic_object()>());
			m.add_function(dynamic_object_get_type_name_interface_name::value,
			               fun(&dynamic_object::type_name));
			m.add_function(dynamic_object_get_attributes_interface_name::value,
			               fun(&dynamic_object::copy_attributes));
			m.add_function(dynamic_object_get_attribute_interface_name::value,
			               fun(static_cast<boxed_value& (dynamic_object::*)(const dynamic_object::attribute_name_type&)>(&dynamic_object::get_attribute)));
			m.add_function(dynamic_object_get_attribute_interface_name::value,
			               fun(static_cast<const boxed_value& (dynamic_object::*)(const dynamic_object::attribute_name_type&) const>(&dynamic_object::get_attribute)));

			//*********************************************
			// exception
			//*********************************************
			m.add_type_info(exception_type_name::value,
			                utility::make_type_info<std::exception>());

			m.add_type_info(exception_logic_error_type_name::value,
			                utility::make_type_info<std::logic_error>());
			// m.add_type_conversion(register_base<std::exception, std::logic_error>());
			register_base<std::exception, std::logic_error>();

			m.add_type_info(exception_out_of_range_type_name::value,
			                utility::make_type_info<std::out_of_range>());
			// m.add_type_conversion(register_base<std::exception, std::out_of_range>());
			// m.add_type_conversion(register_base<std::logic_error, std::out_of_range>());

			m.add_type_info(exception_runtime_error_type_name::value,
			                utility::make_type_info<std::runtime_error>());
			// m.add_type_conversion(register_base<std::exception, std::runtime_error>());
			// runtime_error with a std::string message constructor
			m.add_function(exception_runtime_error_type_name::value,
			               make_constructor<std::runtime_error(const std::string&)>());

			m.add_function(exception_query_interface_name::value,
			               fun([](const std::exception& e) { return std::string{e.what()}; }));
		}
	};
}

#endif // GAL_LANG_KITS_BOOTSTRAP_HPP
