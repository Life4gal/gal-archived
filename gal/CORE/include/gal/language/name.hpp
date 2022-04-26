#pragma once

#ifndef GAL_LANG_LANGUAGE_NAME_HPP
#define GAL_LANG_LANGUAGE_NAME_HPP

#include <utils/template_string.hpp>

namespace gal::lang::lang
{
	//*********************************************
	// keyword
	//*********************************************
	// note: see also ast_node.hpp => is_reserved_name
	// def -> define function
	using keyword_define_name = GAL_UTILS_TEMPLATE_STRING_TYPE("def");
	// class -> define class
	using keyword_class_name = GAL_UTILS_TEMPLATE_STRING_TYPE("class");
	// var -> var declaration
	using keyword_variable_declare_name = GAL_UTILS_TEMPLATE_STRING_TYPE("var");
	// True/False
	using keyword_true_name = GAL_UTILS_TEMPLATE_STRING_TYPE("True");
	using keyword_false_name = GAL_UTILS_TEMPLATE_STRING_TYPE("False");
	// something is global
	using keyword_global_name = GAL_UTILS_TEMPLATE_STRING_TYPE("global");
	// logical and/or
	using keyword_and_name = GAL_UTILS_TEMPLATE_STRING_TYPE("and");
	using keyword_or_name = GAL_UTILS_TEMPLATE_STRING_TYPE("or");
	// if-else
	using keyword_if_name = GAL_UTILS_TEMPLATE_STRING_TYPE("if");
	using keyword_else_name = GAL_UTILS_TEMPLATE_STRING_TYPE("else");
	// for-in
	using keyword_for_in_name = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_2("for", "in");
	// while
	using keyword_while_name = GAL_UTILS_TEMPLATE_STRING_TYPE("while");
	// continue/break
	using keyword_continue_break_name = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_2("pass", "break");
	// match-case-default
	using keyword_match_case_default_name = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_3("match", "=>", "_");
	// placeholder '_'
	using keyword_function_argument_placeholder_name = GAL_UTILS_TEMPLATE_STRING_TYPE("_");
	// try-catch-finally
	using keyword_try_catch_finally_name = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_3("try", "catch", "finally");
	// function guard -> def foo(...) requires xxx
	using keyword_function_guard_name = GAL_UTILS_TEMPLATE_STRING_TYPE("requires");
	// generator a range inline range(begin, end, step = 1)
	using keyword_inline_range_gen_name = GAL_UTILS_TEMPLATE_STRING_TYPE("range");
	// declare an operator def operator==(xxx):
	using keyword_operator_declare_name = GAL_UTILS_TEMPLATE_STRING_TYPE("operator");
	// number inf/nan
	using keyword_number_inf_nan_name = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_2("Inf", "Nan");
	using keyword_magic_line_name = GAL_UTILS_TEMPLATE_STRING_TYPE("__LINE__");
	using keyword_magic_file_name = GAL_UTILS_TEMPLATE_STRING_TYPE("__FILE__");
	using keyword_magic_function_name = GAL_UTILS_TEMPLATE_STRING_TYPE("__FUNC__");
	using keyword_magic_class_name = GAL_UTILS_TEMPLATE_STRING_TYPE("__CLASS__");

	// Not actually keywords below :)
	// every block begin
	using keyword_block_begin_name = GAL_UTILS_TEMPLATE_STRING_TYPE(":");
	// a_class::a_attr
	using keyword_class_accessor_name = GAL_UTILS_TEMPLATE_STRING_TYPE("::");
	// lambda lambda_with_argument -> [](arguments) -> [capture_variable](arguments) / lambda_without_argument -> [] -> [capture_variable]
	using keyword_lambda_name = GAL_UTILS_SYMMETRY_TEMPLATE_STRING_TYPE("[]");
	// pair split {1 : "A"}, {2 : "b"}
	using keyword_pair_splitter_name = GAL_UTILS_TEMPLATE_STRING_TYPE(":");
	// generate a container inline [] -> empty vector / [1, 2, 3, "a", "b", "c"] -> boxed_value vector / [{1 : "A"}, {2 : "b"}] -> map
	using keyword_inline_container_gen_name = GAL_UTILS_SYMMETRY_TEMPLATE_STRING_TYPE("[]");
	// default result name of keyword_magic_function_name if no function found
	using keyword_function_not_found_name = GAL_UTILS_TEMPLATE_STRING_TYPE("NOT_A_VALID_FUNCTION");
	// default result name of keyword_magic_class_name if no class found
	using keyword_class_not_found_name = GAL_UTILS_TEMPLATE_STRING_TYPE("NOT_A_VALID_CLASS");
	// default result filename of an evaluation
	using keyword_inline_eval_filename_name = GAL_UTILS_TEMPLATE_STRING_TYPE("__EVAL__");

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
