#pragma once

#ifndef GAL_LANG_DEBUGGER_HPP
	#define GAL_LANG_DEBUGGER_HPP

	#include <vm/value.hpp>
	#include <vm/vm.hpp>
#include <utils/source_location.hpp>

	#include <string_view>

namespace gal
{
	struct debugger
	{
		static void start_trace(std::string_view tag, const std_source_location& location = std_source_location::current());
		static void trace_message(std::string_view tag, std::string_view message);
		static void end_trace(std::string_view tag, const std_source_location& location = std_source_location::current());

		/**
		 * @brief Prints the stack trace for the current fiber.
		 *
		 * Used when a fiber throws a runtime error which is not caught.
		 */
		static void print_stack_trace(gal_virtual_machine_state& state);

		/**
		 * @brief The "dump" functions are used for debugging GAL itself. Normal code paths
		 * will not call them unless one of the various DEBUG_ flags is enabled.
		 *
		 * Prints a representation of [value] to stdout.
		 */
		static void dump(magic_value value);

		/**
		 * @brief Prints a representation of the bytecode for [function] at instruction [i].
		 */
		static int	dump(gal_virtual_machine_state& state, object_function& function, int i);

		/**
		 * @brief Prints the disassembled code for [function] to stdout.
		 */
		static void dump(gal_virtual_machine_state& state, object_function& function);

		/**
		 * @brief Prints the contents of the current stack for [fiber] to stdout.
		 */
		static void dump(object_fiber& fiber);
	};
}// namespace gal

#endif//GAL_LANG_DEBUGGER_HPP
