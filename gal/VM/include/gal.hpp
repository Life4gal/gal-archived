#pragma once

#ifndef GAL_LANG_VM_GAL_HPP
#define GAL_LANG_VM_GAL_HPP

#ifndef GAL_API
#define GAL_API extern
#endif

#define GAL_LIB_API GAL_API

#include <config/config.hpp>

namespace gal::vm
{
	using user_data_type = void*;

	using null_type = ast::gal_null_type;
	using boolean_type = ast::gal_boolean_type;
	using number_type = ast::gal_number_type;
	using integer_type = int;
	using unsigned_type = unsigned int;

	using index_type = integer_type;
	using stack_size_type = index_type;
	using string_type = const char*;

	enum class thread_status
	{
		ok = 0,
		yield,
		error_run,
		error_syntax,
		error_memory,
		error_error,
		// yielded for a debug breakpoint
		breakpoint,
	};

	constexpr std::uint8_t unknown_object_type = static_cast<std::uint8_t>(-1);
	enum class object_type : std::uint8_t
	{
		null = 0,
		boolean,
		number,

		// all types above this must be value types, all types below this must be GC types
		string,
		table,
		function,
		user_data,
		thread,

		// values below this line are used in object tags but may never show up in magic_value type tags
		prototype,
		upvalue,
		dead_key,

		// the count of magic_value type tags
		tagged_value_count = prototype
	};

	// per thread state, requires destroy manually
	class main_state;
	// main thread's child state, gc object, destroy automatically
	class child_state;

	using internal_function_type = index_type(*)(child_state& state);
	using continuation_function_type = index_type(*)(child_state& state, thread_status status);

	/**
	 * @brief state manipulation
	 */
	namespace state
	{
		GAL_API [[nodiscard]] main_state*  new_state();
		GAL_API void destroy_state(main_state& state);

		GAL_API [[nodiscard]] child_state* new_thread(main_state& state);
		GAL_API [[nodiscard]] main_state&  main_thread(child_state& state);

		GAL_API void reset_thread(child_state& state);
		GAL_API [[nodiscard]] boolean_type is_thread_reset(const child_state& state);
	}// namespace state

	namespace debug
	{
		struct debug_info
		{
			string_type name;
			string_type what;
			string_type source;
			int line_defined;
			int current_line;
			compiler::operand_abc_underlying_type num_upvalues;
			compiler::operand_abc_underlying_type num_params;
			boolean_type is_vararg;
			char short_source[max_id_size];
			user_data_type user_data;
		};

		struct callback_info
		{
			// arbitrary user_data pointer that is never overwritten by GAL
			user_data_type user_data;

			// gets called at safe_points (loop back edges, call/ret, gc) if set
			void (*interrupt)(child_state& state, int gc);
			// gets called when an unprotected error is raised
			void (*panic)(child_state& state, int error_code);

			// gets called when state is created (parent_state == parent) or destroyed (parent_state == nullptr)
			void (*user_thread)(main_state* parent_state, child_state& state);
			// gets called when a string is created; returned atom can be retrieved via to_string_atomic
			std::int16_t (*user_atomic)(string_type string, std::size_t length);

			// gets called when BREAK instruction is encountered
			void (*debug_break)(child_state& state, debug_info& ar);
			// gets called after each instruction in single step mode
			void (*debug_step)(child_state& state, debug_info& ar);
			// gets called when thread execution is interrupted by break in another thread
			void (*debug_interrupt)(child_state& state, debug_info& ar);
			// gets called when handled call results in an error
			void (*debug_handled_error)(child_state& state);
		};

		GAL_API callback_info* callback(child_state& state);
	}
}

#endif // GAL_LANG_VM_GAL_HPP
