#pragma once

#ifndef GAL_LANG_LANGUAGE_NAME_HPP
#define GAL_LANG_LANGUAGE_NAME_HPP

#include <utils/template_string.hpp>

namespace gal::lang::lang
{
	//*********************************************
	// object type & interface
	//*********************************************
	using object_type_name = GAL_UTILS_TEMPLATE_STRING_TYPE("Object");

	using object_self_type_name = GAL_UTILS_TEMPLATE_STRING_TYPE("__this");
	using object_self_name = GAL_UTILS_TEMPLATE_STRING_TYPE("this");

	//*********************************************
	// dynamic object & interface
	//*********************************************
	using dynamic_object_type_name = GAL_UTILS_TEMPLATE_STRING_TYPE("DynamicObject");

	//*********************************************
	// arithmetic operators
	//*********************************************
	using operator_unknown_name = GAL_UTILS_TEMPLATE_STRING_TYPE("unknown");
	using operator_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE("=");
	using operator_assign_if_type_match_name = GAL_UTILS_TEMPLATE_STRING_TYPE(":=");
	using operator_equal_name = GAL_UTILS_TEMPLATE_STRING_TYPE("==");
	using operator_not_equal_name = GAL_UTILS_TEMPLATE_STRING_TYPE("!=");
	using operator_less_than_name = GAL_UTILS_TEMPLATE_STRING_TYPE("<");
	using operator_less_equal_name = GAL_UTILS_TEMPLATE_STRING_TYPE("<=");
	using operator_greater_than_name = GAL_UTILS_TEMPLATE_STRING_TYPE(">");
	using operator_greater_equal_name = GAL_UTILS_TEMPLATE_STRING_TYPE(">=");
	using operator_plus_name = GAL_UTILS_TEMPLATE_STRING_TYPE("+");
	using operator_minus_name = GAL_UTILS_TEMPLATE_STRING_TYPE("-");
	using operator_multiply_name = GAL_UTILS_TEMPLATE_STRING_TYPE("*");
	using operator_divide_name = GAL_UTILS_TEMPLATE_STRING_TYPE("/");
	using operator_remainder_name = GAL_UTILS_TEMPLATE_STRING_TYPE("%");
	using operator_plus_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE("+=");
	using operator_minus_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE("-=");
	using operator_multiply_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE("*=");
	using operator_divide_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE("/=");
	using operator_remainder_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE("%=");
	using operator_bitwise_shift_left_name = GAL_UTILS_TEMPLATE_STRING_TYPE("<<");
	using operator_bitwise_shift_right_name = GAL_UTILS_TEMPLATE_STRING_TYPE(">>");
	using operator_bitwise_and_name = GAL_UTILS_TEMPLATE_STRING_TYPE("&");
	using operator_bitwise_or_name = GAL_UTILS_TEMPLATE_STRING_TYPE("|");
	using operator_bitwise_xor_name = GAL_UTILS_TEMPLATE_STRING_TYPE("^");
	using operator_bitwise_shift_left_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE("<<=");
	using operator_bitwise_shift_right_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE(">>=");
	using operator_bitwise_and_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE("&=");
	using operator_bitwise_or_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE("|=");
	using operator_bitwise_xor_assign_name = GAL_UTILS_TEMPLATE_STRING_TYPE("^=");
	using operator_unary_not_name = GAL_UTILS_TEMPLATE_STRING_TYPE("!");
	using operator_unary_plus_name = GAL_UTILS_TEMPLATE_STRING_TYPE("+");
	using operator_unary_minus_name = GAL_UTILS_TEMPLATE_STRING_TYPE("-");
	using operator_unary_bitwise_complement_name = GAL_UTILS_TEMPLATE_STRING_TYPE("~");

	//*********************************************
	// logical operators
	//*********************************************
	using operator_logical_and_name = GAL_UTILS_TEMPLATE_STRING_TYPE("and");
	using operator_logical_or_name = GAL_UTILS_TEMPLATE_STRING_TYPE("or");
}

#endif // GAL_LANG_LANGUAGE_NAME_HPP
