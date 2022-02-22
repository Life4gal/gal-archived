#pragma once

#ifndef GAL_LANG_LANGUAGE_ALGEBRAIC_HPP
#define GAL_LANG_LANGUAGE_ALGEBRAIC_HPP

#include <utils/hash.hpp>
#include <gal/defines.hpp>

namespace gal::lang
{
	struct algebraic_invoker
	{
		enum class operations
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
		};

		using operation_string_type = std::string_view;

		constexpr static operation_string_type to_string(operations operation) noexcept
		{
			constexpr operation_string_type operation_names[]
			{
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
					{operator_unary_bitwise_complement_name::value}
			};

			return operation_names[static_cast<std::size_t>(operation)];
		}

		constexpr static operations to_operation(const operation_string_type string, const bool is_unary) noexcept
		{
			switch (
				constexpr auto hash = [](const operation_string_type s) constexpr noexcept { return utils::hash_fnv1a<false>(s); };
				hash(string))
			{
					using enum operations;

				case hash(to_string(assign)): { return assign; }
				case hash(to_string(equal)): { return equal; }
				case hash(to_string(not_equal)): { return not_equal; }

				case hash(to_string(less_than)): { return less_than; }
				case hash(to_string(less_equal)): { return less_equal; }
				case hash(to_string(greater_than)): { return greater_than; }
				case hash(to_string(greater_equal)): { return greater_equal; }

				case hash(to_string(plus)): { return is_unary ? unary_plus : plus; }
				case hash(to_string(minus)): { return is_unary ? unary_minus : minus; }
				case hash(to_string(multiply)): { return multiply; }
				case hash(to_string(divide)): { return divide; }
				case hash(to_string(remainder)): { return remainder; }

				case hash(to_string(plus_assign)): { return plus_assign; }
				case hash(to_string(minus_assign)): { return minus_assign; }
				case hash(to_string(multiply_assign)): { return multiply_assign; }
				case hash(to_string(divide_assign)): { return divide_assign; }
				case hash(to_string(remainder_assign)): { return remainder_assign; }

				case hash(to_string(bitwise_shift_left)): { return bitwise_shift_left; }
				case hash(to_string(bitwise_shift_right)): { return bitwise_shift_right; }
				case hash(to_string(bitwise_and)): { return bitwise_and; }
				case hash(to_string(bitwise_or)): { return bitwise_or; }
				case hash(to_string(bitwise_xor)): { return bitwise_xor; }

				case hash(to_string(bitwise_shift_left_assign)): { return bitwise_shift_left_assign; }
				case hash(to_string(bitwise_shift_right_assign)): { return bitwise_shift_right_assign; }
				case hash(to_string(bitwise_and_assign)): { return bitwise_and_assign; }
				case hash(to_string(bitwise_or_assign)): { return bitwise_or_assign; }
				case hash(to_string(bitwise_xor_assign)): { return bitwise_xor_assign; }

				case hash(to_string(unary_not)):
				{
					// todo: assert is_unary is true
					return unary_not;
				}
				case hash(to_string(unary_bitwise_complement)):
				{
					// todo: assert is_unary is true
					return unary_bitwise_complement;
				}

				default: { return unknown; }
			}
		}
	};
}

#endif // GAL_LANG_LANGUAGE_ALGEBRAIC_HPP
