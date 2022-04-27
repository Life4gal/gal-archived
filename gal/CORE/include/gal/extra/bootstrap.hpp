#pragma once

#ifndef GAL_LANG_EXTRA_BOOTSTRAP_HPP
#define GAL_LANG_EXTRA_BOOTSTRAP_HPP

#include <gal/extra/operator_register.hpp>

namespace gal::lang::extra
{
	template<typename T>
		requires std::is_array_v<T>
	void register_array_type(const foundation::string_view_type name, foundation::engine_module& m)
	{
		using return_type = std::remove_extent_t<T>;

		m.add_type_info(name, foundation::make_type_info<T>());

		m.add_function(
				lang::container_subscript_interface_name::value,
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
				lang::container_subscript_interface_name::value,
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
				lang::container_size_interface_name::value,
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
	void register_comparison(foundation::engine_module& m)
	{
		operator_register::register_equal<T>(m);
		operator_register::register_not_equal<T>(m);
		operator_register::register_less_than<T>(m);
		operator_register::register_less_equal<T>(m);
		operator_register::register_greater_than<T>(m);
		operator_register::register_greater_equal<T>(m);
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
		static foundation::boxed_value unknown_assign(foundation::boxed_value lhs, const foundation::boxed_value& rhs)
		{
			if (lhs.is_undefined()) { return lhs.assign(rhs); }
			throw exception::bad_boxed_cast{"boxed_value has a set type already"};
		}

		template<typename T>
			requires std::is_arithmetic_v<T>
		static void register_arithmetic(const foundation::string_view_type name, foundation::engine_module& m)
		{
			m.add_type_info(name, foundation::make_type_info<T>());

			m.add_function(name, default_ctor<T>());
			m.add_function(name, fun([](const foundation::boxed_number& num) { return num.as<T>(); }));
			{
				foundation::string_type n{lang::number_cast_interface_prefix::value};
				n.reserve(n.size() + name.size());
				n.append(name);
				m.add_function(
						std::move(n),
						fun(
								[]([[maybe_unused]] const foundation::string_type& string)
								{
									if constexpr (requires { std::from_chars(static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), std::declval<T&>()); })
									{
										T t;
										std::from_chars(string.data(), string.data() + string.size(), t);
										return t;
									}
									else { throw std::runtime_error{"Parsing given type is not supported yet"}; }
								}));
			}
			{
				foundation::string_type n{lang::number_cast_interface_prefix::value};
				n.reserve(n.size() + name.size());
				n.append(name);
				m.add_function(
						std::move(n),
						fun([](const T t) { return t; }));
			}
		}

		/**
		 * @brief Add all arithmetic operators for PODs
		 */
		static void register_all_arithmetic_operators(foundation::engine_module& m)
		{
			using foundation::boxed_number;

			m.add_function(
					lang::operator_assign_name::value,
					fun(&boxed_number::operator_assign));
			m.add_function(
					lang::operator_equal_name::value,
					fun(&boxed_number::operator_equal));
			m.add_function(
					lang::operator_not_equal_name::value,
					fun(&boxed_number::operator_not_equal));
			m.add_function(
					lang::operator_less_than_name::value,
					fun(&boxed_number::operator_less_than));
			m.add_function(
					lang::operator_less_equal_name::value,
					fun(&boxed_number::operator_less_equal));
			m.add_function(
					lang::operator_greater_than_name::value,
					fun(&boxed_number::operator_greater_than));
			m.add_function(
					lang::operator_greater_equal_name::value,
					fun(&boxed_number::operator_greater_equal));
			m.add_function(
					lang::operator_plus_name::value,
					fun(&boxed_number::operator_plus));
			m.add_function(
					lang::operator_minus_name::value,
					fun(&boxed_number::operator_minus));
			m.add_function(
					lang::operator_multiply_name::value,
					fun(&boxed_number::operator_multiply));
			m.add_function(
					lang::operator_divide_name::value,
					fun(&boxed_number::operator_divide));
			m.add_function(
					lang::operator_remainder_name::value,
					fun(&boxed_number::operator_remainder));
			m.add_function(
					lang::operator_plus_assign_name::value,
					fun(&boxed_number::operator_plus_assign));
			m.add_function(
					lang::operator_minus_assign_name::value,
					fun(&boxed_number::operator_minus_assign));
			m.add_function(
					lang::operator_multiply_assign_name::value,
					fun(&boxed_number::operator_multiply_assign));
			m.add_function(
					lang::operator_divide_assign_name::value,
					fun(&boxed_number::operator_divide_assign));
			m.add_function(
					lang::operator_remainder_assign_name::value,
					fun(&boxed_number::operator_remainder_assign));
			m.add_function(
					lang::operator_bitwise_shift_left_name::value,
					fun(&boxed_number::operator_bitwise_shift_left));
			m.add_function(
					lang::operator_bitwise_shift_right_name::value,
					fun(&boxed_number::operator_bitwise_shift_right));
			m.add_function(
					lang::operator_bitwise_and_name::value,
					fun(&boxed_number::operator_bitwise_and));
			m.add_function(
					lang::operator_bitwise_or_name::value,
					fun(&boxed_number::operator_bitwise_or));
			m.add_function(
					lang::operator_bitwise_xor_name::value,
					fun(&boxed_number::operator_bitwise_xor));
			m.add_function(
					lang::operator_bitwise_shift_left_assign_name::value,
					fun(&boxed_number::operator_bitwise_shift_left_assign));
			m.add_function(
					lang::operator_bitwise_shift_right_assign_name::value,
					fun(&boxed_number::operator_bitwise_shift_right_assign));
			m.add_function(
					lang::operator_bitwise_and_assign_name::value,
					fun(&boxed_number::operator_bitwise_and_assign));
			m.add_function(
					lang::operator_bitwise_or_assign_name::value,
					fun(&boxed_number::operator_bitwise_or_assign));
			m.add_function(
					lang::operator_bitwise_xor_assign_name::value,
					fun(&boxed_number::operator_bitwise_xor_assign));
			m.add_function(
					lang::operator_unary_not_name::value,
					fun(&boxed_number::operator_unary_not));
			m.add_function(
					lang::operator_unary_plus_name::value,
					fun(&boxed_number::operator_unary_plus));
			m.add_function(
					lang::operator_unary_minus_name::value,
					fun(&boxed_number::operator_unary_minus));
			m.add_function(
					lang::operator_unary_bitwise_complement_name::value,
					fun(&boxed_number::operator_unary_bitwise_complement));
		}

	public:
		/**
		 * @brief perform all common bootstrap functions for std::string, void and POD types.
		 */
		static void do_bootstrap(foundation::engine_module& m)
		{
			m.add_function(lang::operator_assign_name::value,
			               fun(&unknown_assign));

			//*********************************************
			// number type & interface
			//*********************************************
			m.add_type_info(lang::number_type_name::value,
			                foundation::make_type_info<foundation::boxed_number>());

			register_arithmetic<std::int8_t>(lang::number_int8_type_name::value, m);
			register_arithmetic<std::uint8_t>(lang::number_uint8_type_name::value, m);
			register_arithmetic<std::int16_t>(lang::number_int16_type_name::value, m);
			register_arithmetic<std::uint16_t>(lang::number_uint16_type_name::value, m);
			register_arithmetic<std::int32_t>(lang::number_int32_type_name::value, m);
			register_arithmetic<std::uint32_t>(lang::number_uint32_type_name::value, m);
			register_arithmetic<std::int64_t>(lang::number_int64_type_name::value, m);
			register_arithmetic<std::uint64_t>(lang::number_uint64_type_name::value, m);
			register_arithmetic<float>(lang::number_float_type_name::value, m);
			register_arithmetic<double>(lang::number_double_type_name::value, m);
			register_arithmetic<long double>(lang::number_long_double_type_name::value, m);

			register_arithmetic<char>(lang::number_char_type_name::value, m);
			register_arithmetic<unsigned char>(lang::number_unsigned_char_type_name::value, m);
			register_arithmetic<wchar_t>(lang::number_wchar_type_name::value, m);
			register_arithmetic<char8_t>(lang::number_char8_type_name::value, m);
			register_arithmetic<char16_t>(lang::number_char16_type_name::value, m);
			register_arithmetic<char32_t>(lang::number_char32_type_name::value, m);
			register_arithmetic<short>(lang::number_short_type_name::value, m);
			register_arithmetic<unsigned short>(lang::number_unsigned_short_type_name::value, m);
			register_arithmetic<int>(lang::number_int_type_name::value, m);
			register_arithmetic<unsigned int>(lang::number_unsigned_int_type_name::value, m);
			register_arithmetic<long>(lang::number_long_type_name::value, m);
			register_arithmetic<unsigned long>(lang::number_unsigned_long_type_name::value, m);
			register_arithmetic<long long>(lang::number_long_long_type_name::value, m);
			register_arithmetic<unsigned long long>(lang::number_unsigned_long_long_type_name::value, m);

			register_all_arithmetic_operators(m);

			m.add_function(lang::operator_to_string_name::value,
			               fun(&foundation::boxed_number::to_string));
		}
	};
}

#endif // GAL_LANG_EXTRA_BOOTSTRAP_HPP
