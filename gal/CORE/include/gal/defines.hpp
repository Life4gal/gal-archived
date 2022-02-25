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
	// keyword
	//*********************************************
	using keyword_define_name = GAL_UTILS_FIXED_STRING_TYPE("def");
	using keyword_function_name = GAL_UTILS_FIXED_STRING_TYPE("fun");
	using keyword_variable_name = GAL_UTILS_FIXED_STRING_TYPE("var");
	using keyword_auto_name = GAL_UTILS_FIXED_STRING_TYPE("auto");
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



	//*********************************************
	// void type
	//*********************************************
	using void_type_name = GAL_UTILS_FIXED_STRING_TYPE("void");

	//*********************************************
	// bool type & interface
	//*********************************************
	using boolean_type_name = GAL_UTILS_FIXED_STRING_TYPE("Bool");

	//*********************************************
	// type info type & interface
	//*********************************************
	using type_info_type_name = GAL_UTILS_FIXED_STRING_TYPE("type_info");

	using type_info_is_void_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_void");
	using type_info_is_arithmetic_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_arithmetic");
	using type_info_is_const_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_const");
	using type_info_is_reference_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_ref");
	using type_info_is_pointer_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_ptr");
	using type_info_is_undefined_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_undef");
	using type_info_bare_equal_interface_name = GAL_UTILS_FIXED_STRING_TYPE("bare_equal");
	using type_info_name_interface_name = GAL_UTILS_FIXED_STRING_TYPE("name");
	using type_info_bare_name_interface_name = GAL_UTILS_FIXED_STRING_TYPE("bare_name");

	//*********************************************
	// object type & interface
	//*********************************************
	using object_type_name = GAL_UTILS_FIXED_STRING_TYPE("Object");

	using object_type_info_interface_name = GAL_UTILS_FIXED_STRING_TYPE("type_info");
	using object_is_undefined_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_undef");
	using object_is_const_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_const");
	using object_is_null_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_null");
	using object_is_reference_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_ref");
	using object_is_pointer_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_ptr");
	using object_is_return_value_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_return_value");
	using object_reset_return_value_interface_name = GAL_UTILS_FIXED_STRING_TYPE("reset_return_value");
	using object_is_type_of_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_type_of");
	using object_get_attribute_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_attr");
	using object_copy_attributes_interface_name = GAL_UTILS_FIXED_STRING_TYPE("copy_attrs");
	using object_clone_attributes_interface_name = GAL_UTILS_FIXED_STRING_TYPE("clone_attrs");

	//*********************************************
	// number type & interface
	//*********************************************
	using number_type_name = GAL_UTILS_FIXED_STRING_TYPE("Number");

	using number_cast_interface_prefix = GAL_UTILS_FIXED_STRING_TYPE("to_");
	using number_int8_type_name = GAL_UTILS_FIXED_STRING_TYPE("i8");
	using number_uint8_type_name = GAL_UTILS_FIXED_STRING_TYPE("u8");
	using number_int16_type_name = GAL_UTILS_FIXED_STRING_TYPE("i16");
	using number_uint16_type_name = GAL_UTILS_FIXED_STRING_TYPE("u16");
	using number_int32_type_name = GAL_UTILS_FIXED_STRING_TYPE("i32");
	using number_uint32_type_name = GAL_UTILS_FIXED_STRING_TYPE("u32");
	using number_int64_type_name = GAL_UTILS_FIXED_STRING_TYPE("i64");
	using number_uint64_type_name = GAL_UTILS_FIXED_STRING_TYPE("u64");
	using number_float_type_name = GAL_UTILS_FIXED_STRING_TYPE("float");
	using number_double_type_name = GAL_UTILS_FIXED_STRING_TYPE("double");
	using number_long_double_type_name = GAL_UTILS_FIXED_STRING_TYPE("long_double");

	using number_char_type_name = GAL_UTILS_FIXED_STRING_TYPE("char");
	using number_unsigned_char_type_name = GAL_UTILS_FIXED_STRING_TYPE("uchar");
	using number_wchar_type_name = GAL_UTILS_FIXED_STRING_TYPE("wchar");
	using number_char8_type_name = GAL_UTILS_FIXED_STRING_TYPE("c8");
	using number_char16_type_name = GAL_UTILS_FIXED_STRING_TYPE("c16");
	using number_char32_type_name = GAL_UTILS_FIXED_STRING_TYPE("c32");
	using number_short_type_name = GAL_UTILS_FIXED_STRING_TYPE("short");
	using number_unsigned_short_type_name = GAL_UTILS_FIXED_STRING_TYPE("ushort");
	using number_int_type_name = GAL_UTILS_FIXED_STRING_TYPE("int");
	using number_unsigned_int_type_name = GAL_UTILS_FIXED_STRING_TYPE("uint");
	using number_long_type_name = GAL_UTILS_FIXED_STRING_TYPE("long");
	using number_unsigned_long_type_name = GAL_UTILS_FIXED_STRING_TYPE("ulong");
	using number_long_long_type_name = GAL_UTILS_FIXED_STRING_TYPE("long_long");
	using number_unsigned_long_long_type_name = GAL_UTILS_FIXED_STRING_TYPE("ulong_long");

	//*********************************************
	// function type & interface
	//*********************************************
	using function_type_name = GAL_UTILS_FIXED_STRING_TYPE("Function");

	using function_get_arity_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_arity");
	using function_equal_interface_name = GAL_UTILS_FIXED_STRING_TYPE("==");
	using function_get_param_types_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_param_types");
	using function_get_contained_functions_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_contained_functions");

	using function_has_guard_interface_name = GAL_UTILS_FIXED_STRING_TYPE("has_guard");
	using function_get_guard_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_guard");

	using function_clone_interface_name = GAL_UTILS_FIXED_STRING_TYPE("clone");

	using assignable_function_type_name = GAL_UTILS_FIXED_STRING_TYPE("AssignableFunction");

	// for dynamic_proxy_function
	using function_has_parse_tree_interface_name = GAL_UTILS_FIXED_STRING_TYPE("has_parse_tree");
	using function_get_parse_tree_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_parse_tree");

	//*********************************************
	// dynamic object & interface
	//*********************************************
	using dynamic_object_type_name = GAL_UTILS_FIXED_STRING_TYPE("DynamicObject");

	using dynamic_object_get_type_name_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_type_name");
	using dynamic_object_get_attributes_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_attrs");
	using dynamic_object_get_attribute_interface_name = GAL_UTILS_FIXED_STRING_TYPE("get_attr");
	using dynamic_object_has_attribute_interface_name = GAL_UTILS_FIXED_STRING_TYPE("has_attr");
	using dynamic_object_set_explicit_interface_name = GAL_UTILS_FIXED_STRING_TYPE("set_explicit");
	using dynamic_object_is_explicit_interface_name = GAL_UTILS_FIXED_STRING_TYPE("is_explicit");
	using dynamic_object_method_missing_interface_name = GAL_UTILS_FIXED_STRING_TYPE("method_missing");

	//*********************************************
	// exception & interface
	//*********************************************
	using exception_type_name = GAL_UTILS_FIXED_STRING_TYPE("exception");
	using exception_logic_error_type_name = GAL_UTILS_FIXED_STRING_TYPE("logic_error");
	using exception_out_of_range_type_name = GAL_UTILS_FIXED_STRING_TYPE("out_of_range");
	using exception_runtime_error_type_name = GAL_UTILS_FIXED_STRING_TYPE("runtime_error");
	using exception_arithmetic_error = GAL_UTILS_FIXED_STRING_TYPE("arithmetic_error");
	using exception_eval_error_type_name = GAL_UTILS_FIXED_STRING_TYPE("eval_error");

	using exception_query_interface_name = GAL_UTILS_FIXED_STRING_TYPE("what");

	using exception_eval_error_reason_interface_name			  = GAL_UTILS_FIXED_STRING_TYPE("reason");
	using exception_eval_error_pretty_print_interface_name = GAL_UTILS_FIXED_STRING_TYPE("pretty_print");
	using exception_eval_error_stack_trace_interface_name = GAL_UTILS_FIXED_STRING_TYPE("stack_trace");

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

	//*********************************************
	// container interface
	//*********************************************
	using container_subscript_interface_name = GAL_UTILS_FIXED_STRING_TYPE("[]");
	using container_size_interface_name = GAL_UTILS_FIXED_STRING_TYPE("size");

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

#endif // GAL_LANG_DEFINES_HPP
