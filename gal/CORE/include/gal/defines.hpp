#pragma once

#ifndef GAL_LANG_DEFINES_HPP
#define GAL_LANG_DEFINES_HPP

#include<vector>
#include<string_view>
#include<string>
#include<memory>

#include<utils/fixed_string.hpp>

#define GAL_LANG_ARITHMETIC_DIVIDE_ZERO_PROTECT

namespace gal::lang
{
	//*********************************************
	// builtin type
	//*********************************************
	using void_type_name = GAL_UTILS_FIXED_STRING_TYPE("void");
	using boolean_type_name = GAL_UTILS_FIXED_STRING_TYPE("Bool");
	using object_type_name = GAL_UTILS_FIXED_STRING_TYPE("Object");
	using number_type_name = GAL_UTILS_FIXED_STRING_TYPE("Number");

	//*********************************************
	// function & interface
	//*********************************************
	using function_type_name = GAL_UTILS_FIXED_STRING_TYPE("Function");
	using assignable_function_type_name = GAL_UTILS_FIXED_STRING_TYPE("AssignableFunction");

	using function_get_arity_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_arity");
	using function_equal_interface_name = GAL_UTILS_FIXED_STRING_TYPE("==");
	using function_get_param_types_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_param_types");
	using function_get_contained_functions_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_contained_functions");

	//*********************************************
	// dynamic object & interface
	//*********************************************
	using dynamic_object_type_name = GAL_UTILS_FIXED_STRING_TYPE("DynamicObject");

	using dynamic_object_get_type_name_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_type_name");
	using dynamic_object_get_attributes_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_attrs");
	using dynamic_object_get_attribute_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_attr");
	using dynamic_object_has_attribute_interface_name = GAL_UTILS_FIXED_STRING_TYPE("has_attrs");
	using dynamic_object_set_explicit_interface_name = GAL_UTILS_FIXED_STRING_TYPE("set_explicit");
	using dynamic_object_is_explicit_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_explicit");

	//*********************************************
	// exception & interface
	//*********************************************
	using exception_type_name = GAL_UTILS_FIXED_STRING_TYPE("exception");
	using exception_logic_error_type_name = GAL_UTILS_FIXED_STRING_TYPE("logic_error");
	using exception_out_of_range_type_name = GAL_UTILS_FIXED_STRING_TYPE("out_of_range");
	using exception_runtime_error_type_name = GAL_UTILS_FIXED_STRING_TYPE("runtime_error");

	using exception_query_interface_name = GAL_UTILS_FIXED_STRING_TYPE("what");

	//*********************************************
	// arithmetic operators
	//*********************************************
	using operator_unknown_name = GAL_UTILS_FIXED_STRING_TYPE("unknown");
	using operator_assign_name = GAL_UTILS_FIXED_STRING_TYPE("=");
	using operator_equal_name = GAL_UTILS_FIXED_STRING_TYPE("==");
	using operator_not_equal_name = GAL_UTILS_FIXED_STRING_TYPE("!=");
	using operator_less_than_name = GAL_UTILS_FIXED_STRING_TYPE("<");
	using operator_less_equal_name = GAL_UTILS_FIXED_STRING_TYPE("<=");
	using operator_greater_than_name = GAL_UTILS_FIXED_STRING_TYPE(">");
	using operator_greater_equal_name = GAL_UTILS_FIXED_STRING_TYPE(">=");
	using operator_plus_name = GAL_UTILS_FIXED_STRING_TYPE("+");
	using operator_minus_name = GAL_UTILS_FIXED_STRING_TYPE("-");
	using operator_multiply_name = GAL_UTILS_FIXED_STRING_TYPE("*");
	using operator_divide_name = GAL_UTILS_FIXED_STRING_TYPE("/");
	using operator_remainder_name = GAL_UTILS_FIXED_STRING_TYPE("%");
	using operator_plus_assign_name = GAL_UTILS_FIXED_STRING_TYPE("+=");
	using operator_minus_assign_name = GAL_UTILS_FIXED_STRING_TYPE("-=");
	using operator_multiply_assign_name = GAL_UTILS_FIXED_STRING_TYPE("*=");
	using operator_divide_assign_name = GAL_UTILS_FIXED_STRING_TYPE("/=");
	using operator_remainder_assign_name = GAL_UTILS_FIXED_STRING_TYPE("%=");
	using operator_bitwise_shift_left_name = GAL_UTILS_FIXED_STRING_TYPE("<<");
	using operator_bitwise_shift_right_name = GAL_UTILS_FIXED_STRING_TYPE(">>");
	using operator_bitwise_and_name = GAL_UTILS_FIXED_STRING_TYPE("&");
	using operator_bitwise_or_name = GAL_UTILS_FIXED_STRING_TYPE("|");
	using operator_bitwise_xor_name = GAL_UTILS_FIXED_STRING_TYPE("^");
	using operator_bitwise_shift_left_assign_name = GAL_UTILS_FIXED_STRING_TYPE("<<=");
	using operator_bitwise_shift_right_assign_name = GAL_UTILS_FIXED_STRING_TYPE(">>=");
	using operator_bitwise_and_assign_name = GAL_UTILS_FIXED_STRING_TYPE("&=");
	using operator_bitwise_or_assign_name = GAL_UTILS_FIXED_STRING_TYPE("|=");
	using operator_bitwise_xor_assign_name = GAL_UTILS_FIXED_STRING_TYPE("^=");
	using operator_unary_not_name = GAL_UTILS_FIXED_STRING_TYPE("!");
	using operator_unary_plus_name = GAL_UTILS_FIXED_STRING_TYPE("+");
	using operator_unary_minus_name = GAL_UTILS_FIXED_STRING_TYPE("-");
	using operator_unary_bitwise_complement_name = GAL_UTILS_FIXED_STRING_TYPE("~");
}

#endif // GAL_LANG_DEFINES_HPP
