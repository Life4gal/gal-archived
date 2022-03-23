#pragma once

#ifndef GAL_LANG_LANGUAGE_NAME_HPP
#define GAL_LANG_LANGUAGE_NAME_HPP

#include <utils/fixed_string.hpp>

namespace gal::lang::lang
{
	//*********************************************
	// keyword
	//*********************************************
	using keyword_define_name = GAL_UTILS_FIXED_STRING_TYPE("def");
	using keyword_function_name = GAL_UTILS_FIXED_STRING_TYPE("fun");
	using keyword_variable_name = GAL_UTILS_FIXED_STRING_TYPE("var");
	using keyword_true_name = GAL_UTILS_FIXED_STRING_TYPE("True");
	using keyword_false_name = GAL_UTILS_FIXED_STRING_TYPE("False");
	using keyword_class_name = GAL_UTILS_FIXED_STRING_TYPE("class");
	using keyword_attribute_name = GAL_UTILS_FIXED_STRING_TYPE("attr");
	using keyword_global_name = GAL_UTILS_FIXED_STRING_TYPE("global");
	using keyword_placeholder_name = GAL_UTILS_FIXED_STRING_TYPE("_");
	using keyword_comma_name = GAL_UTILS_FIXED_STRING_TYPE(",");
	using keyword_while_name = GAL_UTILS_FIXED_STRING_TYPE("while");
	using keyword_for_name = GAL_UTILS_FIXED_STRING_TYPE("for");
	using keyword_break_name = GAL_UTILS_FIXED_STRING_TYPE("break");
	using keyword_if_name = GAL_UTILS_FIXED_STRING_TYPE("if");
	using keyword_else_name = GAL_UTILS_FIXED_STRING_TYPE("else");
	using keyword_logical_and_name = GAL_UTILS_FIXED_STRING_TYPE("and");
	using keyword_logical_or_name = GAL_UTILS_FIXED_STRING_TYPE("or");
	using keyword_return_name = GAL_UTILS_FIXED_STRING_TYPE("return");

	// Not actually keywords below :)
	using keyword_class_scope_name = GAL_UTILS_FIXED_STRING_TYPE("::");
	using keyword_set_guard_name = GAL_UTILS_FIXED_STRING_TYPE("expect");
	using keyword_block_begin_name = GAL_UTILS_FIXED_STRING_TYPE(":");
	using keyword_function_parameter_bracket_name = GAL_UTILS_BILATERAL_FIXED_STRING_TYPE("(", ")");
	using keyword_for_loop_variable_delimiter_name = GAL_UTILS_FIXED_STRING_TYPE(";");

	//*********************************************
	// object type & interface
	//*********************************************
	using object_type_name = GAL_UTILS_FIXED_STRING_TYPE("Object");

	using object_self_type_name = GAL_UTILS_FIXED_STRING_TYPE("__this");
	using object_self_name = GAL_UTILS_FIXED_STRING_TYPE("this");
	using object_clone_interface_name = GAL_UTILS_FIXED_STRING_TYPE("clone");

	//*********************************************
	// dynamic object & interface
	//*********************************************
	using dynamic_object_type_name = GAL_UTILS_FIXED_STRING_TYPE("DynamicObject");

	//*********************************************
	// arithmetic operators
	//*********************************************
	using operator_unknown_name = GAL_UTILS_FIXED_STRING_TYPE("unknown");
	using operator_assign_name = GAL_UTILS_FIXED_STRING_TYPE("=");
	using operator_assign_if_type_match_name = GAL_UTILS_FIXED_STRING_TYPE(":=");
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

	//*********************************************
	// container interface
	//*********************************************
	using container_subscript_interface_name = GAL_UTILS_FIXED_STRING_TYPE("[]");
	using container_size_interface_name = GAL_UTILS_FIXED_STRING_TYPE("size");
	using container_range_interface_name = GAL_UTILS_FIXED_STRING_TYPE("range");
	using container_empty_interface_name = GAL_UTILS_FIXED_STRING_TYPE("empty");
	using container_front_interface_name = GAL_UTILS_FIXED_STRING_TYPE("front");
	using container_pop_front_interface_name = GAL_UTILS_FIXED_STRING_TYPE("pop_front");

	//*********************************************
	// common operators & interface
	//*********************************************
	using operator_to_string_name = GAL_UTILS_FIXED_STRING_TYPE("to_string");

	// must throw boxed_value
	using operator_raise_exception_name = GAL_UTILS_FIXED_STRING_TYPE("throw");

	using operator_print_name = GAL_UTILS_FIXED_STRING_TYPE("print");
	using operator_println_name = GAL_UTILS_FIXED_STRING_TYPE("println");

	// make a function bound some args(return a object(boxed_value))
	using operator_bind_name = GAL_UTILS_FIXED_STRING_TYPE("bind");

	// true if the two boxed_values share the same internal type
	using operator_type_match_name = GAL_UTILS_FIXED_STRING_TYPE("type_match");

	using operator_range_generate_name = GAL_UTILS_FIXED_STRING_TYPE("range_generate");

	using file_position_type_name = GAL_UTILS_FIXED_STRING_TYPE("FilePosition");
	using file_position_line_interface_name = GAL_UTILS_FIXED_STRING_TYPE("line");
	using file_position_column_interface_name = GAL_UTILS_FIXED_STRING_TYPE("column");

	using ast_node_type_name = GAL_UTILS_FIXED_STRING_TYPE("ASTNode");
	using ast_node_type_interface_name = GAL_UTILS_FIXED_STRING_TYPE("type");
	using ast_node_text_interface_name = GAL_UTILS_FIXED_STRING_TYPE("text");
	using ast_node_location_begin_interface_name = GAL_UTILS_FIXED_STRING_TYPE("begin");
	using ast_node_location_end_interface_name = GAL_UTILS_FIXED_STRING_TYPE("end");
	using ast_node_filename_interface_name = GAL_UTILS_FIXED_STRING_TYPE("filename");
	using ast_node_to_string_interface_name = GAL_UTILS_FIXED_STRING_TYPE("to_string");
	using ast_node_children_interface_name = GAL_UTILS_FIXED_STRING_TYPE("children");
}

#endif // GAL_LANG_LANGUAGE_NAME_HPP
