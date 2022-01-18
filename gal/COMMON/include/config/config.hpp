#pragma once

#ifndef GAL_LANG_COMMON_CONFIG_HPP
	#define GAL_LANG_COMMON_CONFIG_HPP

#include <cstddef>
#include <cstdint>

namespace gal
{
	namespace ast
	{
		using gal_null_type = std::nullptr_t;
		using gal_boolean_type = bool;
		using gal_number_type = double;
	}

	namespace compiler
	{
		using operand_underlying_type = std::uint32_t;
		using operand_abc_underlying_type = std::uint8_t;
		using operand_d_underlying_type = std::int16_t;
		using operand_e_underlying_type = std::int32_t;
		using operand_aux_underlying_type = operand_underlying_type;

		using stack_size_type = std::uint32_t;
		using register_size_type = stack_size_type;
		using debug_pc_type = std::uint32_t;
		using register_type = operand_abc_underlying_type;
		using baseline_delta_type = std::uint8_t;
	}

	namespace vm
	{
		using instruction_type = compiler::operand_underlying_type;

		/**
		 * @brief The maximum size for the description of the source
		 */
		constexpr auto max_id_size						 = 256;

		/**
		 * @brief The desired top heap size in relation to the live heap size at the end of the GC cycle
		 *
		 * @note 200% (allow heap to double compared to live heap size)
		 */
		constexpr auto default_gc_goal							 = 200;

		/**
		 * @brief The default speed of garbage collection relative to memory allocation
		 *
		 * @note Every GC_STEP_SIZE KB allocated, incremental collector collects GC_STEP_SIZE times GC_STEP_MULTIPLE% bytes.
		 */
		constexpr auto default_gc_step_multiple						 = 200;

		/**
		 * @brief GC runs every 1 KB of memory allocation
		 */
		constexpr auto default_gc_step_size							 = 1;

		/**
		 * @brief The guaranteed number of stack slots available to a internal function
		 */
		constexpr auto min_stack_size					 = 20;

		/**
		 * @brief The number of stack slots that a internal function can use
		 */
		constexpr auto max_stack_size					 = 8000;

		/**
		 * @brief The number of nested calls
		 */
		constexpr auto max_call_size					 = 20000;

		/**
		 * @brief The maximum depth for nested internal calls; this limit depends on native stack size
		 */
		constexpr auto max_internal_call				 = 200;

		/**
		 * @brief The buffer size used for on-stack string operations; this limit depends on native stack size
		 */
		constexpr auto buffer_size						 = 512;

		using user_data_tag_type						 = std::uint8_t;
		/**
		 * @brief The limit of valid user data tag
		 */
		constexpr user_data_tag_type user_data_tag_limit = 128;
		/**
		 * @brief Special tag value is used for user data with inline destructor
		 */
		constexpr user_data_tag_type user_data_tag_inline_destructor = user_data_tag_limit;

		/**
		 * @brief The upper bound for number of size classes used by page allocator
		 */
		constexpr auto				 size_classes		 = 32;

		/**
		 * @brief maximum number of captures supported by pattern matching
		 */
		constexpr auto				 max_captures		 = 32;
	}// namespace vm
}

#endif // GAL_LANG_COMMON_CONFIG_HPP
