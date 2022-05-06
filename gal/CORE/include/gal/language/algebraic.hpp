#pragma once

#ifndef GAL_LANG_LANGUAGE_ALGEBRAIC_HPP
#define GAL_LANG_LANGUAGE_ALGEBRAIC_HPP

#include <utils/assert.hpp>
#include <utils/hash.hpp>
#include <gal/foundation/string.hpp>
#include <gal/language/name.hpp>

namespace gal::lang::lang
{
	enum class algebraic_operations
	{
		unknown,

		// =
		assign,
		// ==
		equal,
		// !=
		not_equal,

		// <
		less_than,
		// <=
		less_equal,
		// >
		greater_than,
		// >=
		greater_equal,

		// +
		plus,
		// -
		minus,
		// *
		multiply,
		// /
		divide,
		// %
		remainder,

		// +=
		plus_assign,
		// -=
		minus_assign,
		// *=
		multiply_assign,
		// /=
		divide_assign,
		// %=
		remainder_assign,

		// <<
		bitwise_shift_left,
		// >>
		bitwise_shift_right,
		// &
		bitwise_and,
		// |
		bitwise_or,
		// ^
		bitwise_xor,

		// <<=
		bitwise_shift_left_assign,
		// >>=
		bitwise_shift_right_assign,
		// &=
		bitwise_and_assign,
		// |=
		bitwise_or_assign,
		// ^=
		bitwise_xor_assign,

		// !
		unary_not,
		// +
		unary_plus,
		// -
		unary_minus,
		// ~
		unary_bitwise_complement,

		operations_size
	};

	using algebraic_operation_name_type = foundation::string_view_type;

	constexpr algebraic_operation_name_type algebraic_name(algebraic_operations operation) noexcept
	{
		constexpr algebraic_operation_name_type operation_names[]{
				{operator_unknown_name::value},

				{operator_assign_name::value},
				{operator_equal_name::value},
				{operator_not_equal_name::value},

				{operator_less_than_name::value},
				{operator_less_equal_name::value},
				{operator_greater_than_name::value},
				{operator_greater_equal_name::value},

				{operator_plus_name::value},
				{operator_minus_name::value},
				{operator_multiply_name::value},
				{operator_divide_name::value},
				{operator_remainder_name::value},

				{operator_plus_assign_name::value},
				{operator_minus_assign_name::value},
				{operator_multiply_assign_name::value},
				{operator_divide_assign_name::value},
				{operator_remainder_assign_name::value},

				{operator_bitwise_shift_left_name::value},
				{operator_bitwise_shift_right_name::value},
				{operator_bitwise_and_name::value},
				{operator_bitwise_or_name::value},
				{operator_bitwise_xor_name::value},

				{operator_bitwise_shift_left_assign_name::value},
				{operator_bitwise_shift_right_assign_name::value},
				{operator_bitwise_and_assign_name::value},
				{operator_bitwise_or_assign_name::value},
				{operator_bitwise_xor_assign_name::value},

				{operator_unary_not_name::value},
				{operator_unary_plus_name::value},
				{operator_unary_minus_name::value},
				{operator_unary_bitwise_complement_name::value}};

		static_assert(std::size(operation_names) == static_cast<std::size_t>(algebraic_operations::operations_size));

		return operation_names[static_cast<std::size_t>(operation)];
	}

	constexpr algebraic_operations algebraic_operation(const algebraic_operation_name_type name, const bool is_unary = false) noexcept
	{
		switch (
			constexpr auto hash = [](const algebraic_operation_name_type s) constexpr noexcept { return utils::hash_fnv1a<false>(s); };
			hash(name))
		{
				using enum algebraic_operations;

			case hash(algebraic_name(assign)): { return assign; }
			case hash(algebraic_name(equal)): { return equal; }
			case hash(algebraic_name(not_equal)): { return not_equal; }

			case hash(algebraic_name(less_than)): { return less_than; }
			case hash(algebraic_name(less_equal)): { return less_equal; }
			case hash(algebraic_name(greater_than)): { return greater_than; }
			case hash(algebraic_name(greater_equal)): { return greater_equal; }

			case hash(algebraic_name(plus)): { return is_unary ? unary_plus : plus; }
			case hash(algebraic_name(minus)): { return is_unary ? unary_minus : minus; }
			case hash(algebraic_name(multiply)): { return multiply; }
			case hash(algebraic_name(divide)): { return divide; }
			case hash(algebraic_name(remainder)): { return remainder; }

			case hash(algebraic_name(plus_assign)): { return plus_assign; }
			case hash(algebraic_name(minus_assign)): { return minus_assign; }
			case hash(algebraic_name(multiply_assign)): { return multiply_assign; }
			case hash(algebraic_name(divide_assign)): { return divide_assign; }
			case hash(algebraic_name(remainder_assign)): { return remainder_assign; }

			case hash(algebraic_name(bitwise_shift_left)): { return bitwise_shift_left; }
			case hash(algebraic_name(bitwise_shift_right)): { return bitwise_shift_right; }
			case hash(algebraic_name(bitwise_and)): { return bitwise_and; }
			case hash(algebraic_name(bitwise_or)): { return bitwise_or; }
			case hash(algebraic_name(bitwise_xor)): { return bitwise_xor; }

			case hash(algebraic_name(bitwise_shift_left_assign)): { return bitwise_shift_left_assign; }
			case hash(algebraic_name(bitwise_shift_right_assign)): { return bitwise_shift_right_assign; }
			case hash(algebraic_name(bitwise_and_assign)): { return bitwise_and_assign; }
			case hash(algebraic_name(bitwise_or_assign)): { return bitwise_or_assign; }
			case hash(algebraic_name(bitwise_xor_assign)): { return bitwise_xor_assign; }

			case hash(algebraic_name(unary_not)):
			{
				gal_assert(is_unary);
				return unary_not;
			}
			case hash(algebraic_name(unary_bitwise_complement)):
			{
				gal_assert(is_unary);
				return unary_bitwise_complement;
			}

			default: { return unknown; }
		}
	}
}

#endif // GAL_LANG_LANGUAGE_ALGEBRAIC_HPP
