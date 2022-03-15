#pragma once

#ifndef GAL_LANG_KITS_OPERATORS_HPP
#define GAL_LANG_KITS_OPERATORS_HPP

#include <gal/defines.hpp>
#include <gal/kits/register_function.hpp>
#include <gal/kits/dispatch.hpp>

namespace gal::lang::kits::detail
{
	template<typename T>
	void register_assign(engine_module& m)
	{
		m.add_function(
				operator_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs = rhs; }));
	}

	template<typename T>
	void register_equal(engine_module& m)
	{
		m.add_function(
				operator_equal_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs == rhs; }));
	}

	template<typename T>
	void register_not_equal(engine_module& m)
	{
		m.add_function(
				operator_not_equal_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs != rhs; }));
	}

	template<typename T>
	void register_less_than(engine_module& m)
	{
		m.add_function(
				operator_less_than_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs < rhs; }));
	}

	template<typename T>
	void register_less_equal(engine_module& m)
	{
		m.add_function(
				operator_less_equal_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs <= rhs; }));
	}

	template<typename T>
	void register_greater_than(engine_module& m)
	{
		m.add_function(
				operator_greater_than_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs > rhs; }));
	}

	template<typename T>
	void register_greater_equal(engine_module& m)
	{
		m.add_function(
				operator_greater_equal_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs >= rhs; }));
	}

	template<typename T>
	void register_plus(engine_module& m)
	{
		m.add_function(
				operator_plus_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs + rhs; }));
	}

	template<typename T>
	void register_minus(engine_module& m)
	{
		m.add_function(
				operator_minus_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs - rhs; }));
	}

	template<typename T>
	void register_multiply(engine_module& m)
	{
		m.add_function(
				operator_multiply_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs * rhs; }));
	}

	template<typename T>
	void register_divide(engine_module& m)
	{
		m.add_function(
				operator_divide_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs / rhs; }));
	}

	template<typename T>
	void register_remainder(engine_module& m)
	{
		m.add_function(
				operator_remainder_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs % rhs; }));
	}

	template<typename T>
	void register_plus_assign(engine_module& m)
	{
		m.add_function(
				operator_plus_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs += rhs; }));
	}

	template<typename T>
	void register_minus_assign(engine_module& m)
	{
		m.add_function(
				operator_minus_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs -= rhs; }));
	}

	template<typename T>
	void register_multiply_assign(engine_module& m)
	{
		m.add_function(
				operator_multiply_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs *= rhs; }));
	}

	template<typename T>
	void register_divide_assign(engine_module& m)
	{
		m.add_function(
				operator_divide_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs /= rhs; }));
	}

	template<typename T>
	void register_remainder_assign(engine_module& m)
	{
		m.add_function(
				operator_remainder_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs %= rhs; }));
	}

	template<typename T>
	void register_bitwise_shift_left(engine_module& m)
	{
		m.add_function(
				operator_bitwise_shift_left_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs << rhs; }));
	}

	template<typename T>
	void register_bitwise_shift_right(engine_module& m)
	{
		m.add_function(
				operator_bitwise_shift_right_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs >> rhs; }));
	}

	template<typename T>
	void register_bitwise_and(engine_module& m)
	{
		m.add_function(
				operator_bitwise_and_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs & rhs; }));
	}

	template<typename T>
	void register_bitwise_or(engine_module& m)
	{
		m.add_function(
				operator_bitwise_or_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs | rhs; }));
	}

	template<typename T>
	void register_bitwise_xor(engine_module& m)
	{
		m.add_function(
				operator_bitwise_xor_name::value,
				fun([](const T& lhs, const T& rhs) { return lhs ^ rhs; }));
	}

	template<typename T>
	void register_bitwise_shift_left_assign(engine_module& m)
	{
		m.add_function(
				operator_bitwise_shift_left_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs <<= rhs; }));
	}

	template<typename T>
	void register_bitwise_shift_right_assign(engine_module& m)
	{
		m.add_function(
				operator_bitwise_shift_right_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs >>= rhs; }));
	}

	template<typename T>
	void register_bitwise_and_assign(engine_module& m)
	{
		m.add_function(
				operator_bitwise_and_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs &= rhs; }));
	}

	template<typename T>
	void register_bitwise_or_assign(engine_module& m)
	{
		m.add_function(
				operator_bitwise_or_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs |= rhs; }));
	}

	template<typename T>
	void register_bitwise_xor_assign(engine_module& m)
	{
		m.add_function(
				operator_bitwise_xor_assign_name::value,
				fun([](T& lhs, const T& rhs) -> T& { return lhs ^= rhs; }));
	}

	template<typename T>
	void register_unary_not(engine_module& m)
	{
		m.add_function(
				operator_unary_not_name::value,
				fun([](const T& self) { return !self; }));
	}

	template<typename T>
	void register_unary_plus(engine_module& m)
	{
		m.add_function(
				operator_unary_plus_name::value,
				fun([](const T& self) { return +self; }));
	}

	template<typename T>
	void register_unary_minus(engine_module& m)
	{
		m.add_function(
				operator_unary_minus_name::value,
				fun([](const T& self) { return -self; }));
	}

	template<typename T>
	void register_unary_bitwise_complement(engine_module& m)
	{
		m.add_function(
				operator_unary_bitwise_complement_name::value,
				fun([](const T& self) { return ~self; }));
	}
}

#endif // GAL_LANG_KITS_OPERATORS_HPP
