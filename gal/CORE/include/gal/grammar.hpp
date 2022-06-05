#pragma once

#ifndef GAL_LANG_GRAMMAR_HPP
#define GAL_LANG_GRAMMAR_HPP

namespace gal::lang::grammar
{
	using index_type = std::size_t;

	/**
	 * @brief nothing, just an identifier.
	 *
	 * identifier -> boxed_value name
	 */
	struct id_ast_node { };

	/**
	 * @brief nothing, just a boxed_value.
	 *
	 * value -> the constant boxed_value
	 */
	struct constant_ast_node { };

	/**
	 * @brief Has no meaning in itself.
	 *
	 * child =>
	 *
	 * 0: id_ast_node -> reference target
	 */
	struct reference_ast_node
	{
		constexpr static index_type identifier_index = 0;
	};

	/**
	 * @brief Holds an identifier that determines what kind of unary operation.
	 *
	 * identifier -> operation (literal) name
	 *
	 * child =>
	 *
	 * 0: id_ast_node -> operation target
	 */
	struct unary_operator_ast_node
	{
		constexpr static index_type index = 0;
	};

	/**
	 * @brief Binary operation with right operand folded.
	 *
	 * identifier -> operation (literal) name
	 * params -> hold the right operand
	 *
	 * child =>
	 *
	 * 0: id_ast_node -> left-hand-side operation target
	 */
	struct fold_right_binary_operator_ast_node
	{
		constexpr static index_type lhs_index = 0;
	};

	/**
	 * @brief Binary operation.
	 *
	 * identifier -> operation (literal) name
	 *
	 * children =>
	 *
	 * 0: id_ast_node -> left-hand-side operation target
	 *
	 * 1: id_ast_node -> right-hand-side operation target
	 */
	struct binary_operator_ast_node
	{
		constexpr static index_type lhs_index = 0;
		constexpr static index_type rhs_index = 1;
	};

	/**
	 * @brief A function call.
	 * @note see also arg_list_ast_node
	 *
	 * children =>
	 *
	 * 0: id_ast_node -> function (literal) name
	 * 0: dot_access_ast_node -> object.function(arguments)
	 *
	 * 1: arg_list_node -> function parameters
	 */
	struct fun_call_ast_node
	{
		constexpr static index_type function_index = 0;
		constexpr static index_type arg_list_index = 1;
	};

	/**
	 * @brief Basically a '[]' function call.
	 *
	 * identifier -> function (literal) name, should be '[]'
	 *
	 * children =>
	 *
	 * 0: id_ast_node/other types return nodes that support the '[]' operation -> operation target
	 *
	 * 1: some nodes that will generate an 'index' -> index
	 */
	struct array_access_ast_node
	{
		constexpr static index_type operation_target_index = 0;
		constexpr static index_type operation_parameter_index = 1;
	};

	/**
	 * @brief A function call separated by '.', the left is the called target and the right is the called function.
	 *
	 * function_name -> function (literal) name, base on child_node(function_name_index)
	 *
	 * 0: id_ast_node/other types return nodes that support the '.' operation -> operation target
	 *
	 *	children in general =>
	 *
	 * 1: id_ast_node -> function (literal) name
	 *
	 * children in complex situations =>
	 *
	 * 1: fun_call_ast_node/array_access_ast_node
	 */
	struct dot_access_ast_node
	{
		constexpr static index_type target_index = 0;

		constexpr static index_type function_index = 1;

		static_assert(fun_call_ast_node::function_index == array_access_ast_node::operation_target_index);
		constexpr static index_type function_secondary_index = fun_call_ast_node::function_index;
		static_assert(fun_call_ast_node::arg_list_index == array_access_ast_node::operation_parameter_index);
		constexpr static index_type function_parameter_index = fun_call_ast_node::arg_list_index;
	};

	/**
	 * @brief Represents a variable, optionally with type identity.
	 *
	 * 0: id_ast_node -> type name (if a type is identified) or variable name
	 *
	 * 1: id_ast_node -> variable name (if type is identified)
	 */
	struct arg_ast_node
	{
		constexpr static index_type type_or_name_index = 0;
		// check if there is an identity type before using this value
		constexpr static index_type name_index = 1;
	};

	/**
	 * @brief Represents a list of parameters.
	 */
	struct arg_list_ast_node { };

	/**
	 * @brief Some kind of operation performed on the left and right variables.
	 * In particular, the assignment operator is supported.
	 *
	 * @note see also global_decl_ast_node
	 *
	 * identifier -> operation (literal) name
	 *
	 * children =>
	 *
	 * 0: id_ast_node -> left-hand-side operation target
	 *
	 * 0: global_decl_ast_node
	 *
	 * 1: id_ast_node -> right-hand-side operation target
	 */
	struct equation_ast_node
	{
		constexpr static index_type lhs_index = 0;
		constexpr static index_type rhs_index = 1;
	};

	/**
	 * @brief Represents a global variable.
	 *
	 * child =>
	 *
	 * 0: id_ast_node -> variable name
	 *
	 * 0: reference_ast_node -> reference variable name
	 */
	struct global_decl_ast_node
	{
		constexpr static index_type index = 0;
		// if it is a reference
		constexpr static index_type secondary_index = reference_ast_node::identifier_index;
	};

	/**
	 * @brief Represents a local variable.
	 *
	 * child =>
	 *
	 * 0: id_ast_node -> variable name
	 */
	struct var_decl_ast_node
	{
		constexpr static index_type index = 0;
	};

	/**
	 * @brief A assignment. Basically optimized from equation_ast_node.
	 *
	 * children =>
	 *
	 * 0: id_ast_node -> left-hand-side operation target
	 *
	 * 1: id_ast_node -> right-hand-side operation target
	 */
	struct assign_decl_ast_node
	{
		constexpr static index_type lhs_index = 0;
		constexpr static index_type rhs_index = 1;
	};

	/**
	 * @brief Represents the definition of a class.
	 *
	 * children =>
	 *
	 * 0: id_ast_node -> class (literal) name
	 *
	 * 1: block_ast_node -> class body definitions with class (such as member variables and member functions)
	 */
	struct class_decl_ast_node
	{
		constexpr static index_type name_index = 0;
		constexpr static index_type body_index = 1;
	};

	/**
	 * @brief Represents the definition of a class member variable.
	 *
	 * children =>
	 *
	 * 0: id_ast_node -> class (literal) name
	 *
	 * 1: id_ast_node -> class member variable (literal) name
	 */
	struct member_decl_ast_node
	{
		constexpr static index_type class_name_index = 0;
		constexpr static index_type member_name_index = 1;
	};

	/**
	 * @brief Represents the definition of a function, in particular, does not include member functions, see method_ast_node for member functions.
	 *
	 * body_node -> function body
	 *
	 * guard_node -> optional guard node (can be any operation) -> used to check the validity of parameters
	 *
	 * children =>
	 *
	 * 0: id_ast_node -> function (literal) name
	 *
	 * 1: arg_list_ast_node -> function parameters (optional)
	 */
	struct def_ast_node
	{
		constexpr static index_type function_name_index = 0;
		constexpr static index_type arg_list_or_guard_or_body_index = 1;

		// The following values represent the indices of the following nodes in the children given when constructing the def_ast_node

		// check if there is a arg_list before using this value
		constexpr static index_type guard_or_body_index = 2;
		// check if there is a guard before using this value
		constexpr static index_type body_index = 3;
	};

	/**
	 * @brief Similar to def_ast_node, but specifically represents a class member function.
	 *
	 * body_node -> function body
	 *
	 * guard_node -> optional guard node (can be any operation) -> used to check the validity of parameters
	 *
	 * children =>
	 *
	 * 0: id_ast_node -> class name
	 *
	 * 1: id_ast_node -> function name (defined inside the class) or class name (defined outside the class)
	 *
	 * 2: id_ast_node -> function name (defined outside the function) (def class_name::function_name(arg_list) { function_body})
	 *
	 * 2: arg_list_ast_node -> function parameters (optional)
	 *
	 */
	struct method_ast_node
	{
		constexpr static index_type class_name_index = 0;
		constexpr static index_type function_name_index = 1;
		constexpr static index_type arg_list_index = 2;
	};

	/**
	 * @brief Represents an anonymous function definition.
	 *
	 * lambda_node -> lambda function body
	 *
	 * children =>
	 *
	 * 0: arg_list_ast_node -> list of captured variables
	 *
	 * 1: arg_list_ast_node -> function parameters (optional)
	 *
	 * 1/2: block_ast_node -> function body
	 */
	struct lambda_ast_node
	{
		constexpr static index_type capture_list_index = 0;

		constexpr static index_type function_parameter_or_body_index = 1;

		// The following values represent the indices of the following nodes in the children given when constructing the lambda_ast_node

		constexpr static index_type body_index = 2;
	};

	/**
	 * @brief Usually represents a function body or a class definition.
	 * There is nothing to pay attention to, just eval all the nodes in the block in turn, and then return the evaluation result of the last node.
	 */
	struct block_ast_node { };

	/**
	 * @brief Represents an if branch judgment.
	 *
	 * @note 'if' statement is always assumed with at least one trailing 'else' statement, if not it is filled with noop_ast_node.
	 * @note Although if(init; cond) is syntactically supported, init is not actually evaluated in an 'if' statement.
	 *
	 * children =>
	 *
	 * 0: condition
	 *
	 * 1: block_ast_node -> true branch
	 *
	 * 2: block_ast_node -> false branch
	 */
	struct if_ast_node
	{
		constexpr static index_type condition_index = 0;
		constexpr static index_type true_branch_index = 1;
		constexpr static index_type false_branch_index = 2;
	};

	/**
	 * @brief Represents a while loop.
	 *
	 * children =>
	 *
	 * 0: condition
	 *
	 * 1: block_ast_node -> loop body
	 */
	struct while_ast_node
	{
		constexpr static index_type condition_index = 0;
		constexpr static index_type body_index = 1;
	};

	/**
	 * @brief Represents a for loop.
	 *
	 * @note The GAL does not support the traditional for(init;cond;iteration) loop, but any container that supports (implements) 'view' can be iterated over.
	 *
	 * children =>
	 *
	 * 0: loop variable name
	 *
	 * 1: loop range/container variable name
	 *
	 * 2: block_ast_node -> loop body
	 */
	struct ranged_for_ast_node
	{
		constexpr static index_type loop_variable_name_index = 0;
		constexpr static index_type loop_range_name_index = 1;
		constexpr static index_type body_index = 2;
	};

	/**
	 * @brief Represents a return statement.
	 *
	 * @note return is optional with an operation, returns the result of the operation, or directly returns void if there is no operation.
	 */
	struct return_ast_node
	{
		constexpr static index_type operation_index = 0;
	};

	/**
	 * @brief Represents a matching default branch statement.
	 *
	 * child =>
	 *
	 * 0: block_ast_node -> default branch body
	 */
	struct match_default_ast_node
	{
		constexpr static index_type body_index = 0;
	};

	/**
	 * @brief Represents a matching case branch statement.
	 *
	 * @note Although match_case_ast_node holds the match condition child node, it does not evaluate itself, but passes it to match_ast_node.
	 *
	 * children =>
	 *
	 * 0: match value
	 *
	 * 1: block_ast_node -> branch body
	 */
	struct match_case_ast_node
	{
		constexpr static index_type match_value_index = 0;
		constexpr static index_type body_index = 1;
	};

	/**
	 * @brief Represents a matching branch statement.
	 *
	 * children =>
	 *
	 * 0: match value
	 *
	 * 1~n: match_case_ast_node (0~n)
	 *
	 * 1~n: match_default_ast_node (0~1)
	 */
	struct match_ast_node
	{
		constexpr static index_type match_value_index = 0;
	};

	/**
	 * @brief Represents a logical AND statement.
	 *
	 * children =>
	 *
	 * 0: left-hand-side operation
	 *
	 * 1: right-hand-side operation
	 */
	struct logical_and_ast_node
	{
		constexpr static index_type lhs_index = 0;
		constexpr static index_type rhs_index = 1;
	};

	using logical_or_ast_node = logical_and_ast_node;

	/**
	 * @brief Syntactic sugar for creating a list container.
	 *
	 * child =>
	 *
	 * 0: arg_list_ast_node -> Various possible parameters (operations)
	 */
	struct inline_list_ast_node
	{
		constexpr static index_type arg_list_index = 0;
	};

	/**
	 * @brief Represents a pair of map.
	 *
	 * children =>
	 *
	 * 0: pair key operation
	 *
	 * 1: pair value operation
	 */
	struct map_pair_ast_node
	{
		constexpr static index_type key_index = 0;
		constexpr static index_type value_index = 1;
	};

	/**
	 * @brief Syntactic sugar for creating a map container.
	 *
	 * child =>
	 *
	 * 0: arg_list_ast_node -> Various possible parameters (pairs)
	 */
	struct inline_map_ast_node
	{
		constexpr static index_type arg_list_index = 0;
	};

	/**
	 * @brief Represents the 'catch' branch of a 'try'.
	 *
	 * children =>
	 *
	 * 0: arg_ast_node -> catch branch argument (optional)
	 *
	 * 1: block_ast_node -> catch branch body
	 */
	struct try_catch_ast_node
	{
		constexpr static index_type argument_or_body_index = 0;
		// check if there is an argument before using this value
		constexpr static index_type body_index = 1;
	};

	/**
	 * @brief Represents the 'finally' branch of a 'try'.
	 *
	 * child =>
	 *
	 * 0: block_ast_node -> finally branch body
	 */
	struct try_finally_ast_node
	{
		constexpr static index_type body_index = 0;
	};

	/**
	 * @brief Represents a 'try' statement.
	 *
	 * children =>
	 *
	 * 0: block_ast_node -> a block of statements that needs to be surrounded by try.
	 *
	 * 1~n: try_catch_ast_node (0~n)
	 *
	 * 1~n: try_finally_ast_node (0~1)
	 */
	struct try_ast_node
	{
		constexpr static index_type body_index = 0;
	};
}

#endif // GAL_LANG_GRAMMAR_HPP
