#pragma once

#ifndef GAL_LANG_VM_GAL_HPP
#define GAL_LANG_VM_GAL_HPP

#ifndef GAL_API
#define GAL_API extern
#endif

#define GAL_LIB_API GAL_API

#include <config/config.hpp>

namespace gal::vm_dev
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

	enum class vm_status
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

	constexpr std::uint8_t unknown_object_type(-1);

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
	using continuation_function_type = index_type(*)(child_state& state, vm_status status);

	/**
	 * @brief state manipulation
	 */
	namespace state
	{
		GAL_API main_state* new_state(user_data_type user_data);
		GAL_API void destroy_state(main_state& state);

		GAL_API child_state* new_thread(main_state& state);
		GAL_API main_state* main_thread(child_state& state);

		GAL_API void reset_thread(main_state& state);
		GAL_API void reset_thread(child_state& state);
		GAL_API boolean_type is_thread_reset(main_state& state);
		GAL_API boolean_type is_thread_reset(child_state& state);
	}// namespace state
}

#endif // GAL_LANG_VM_GAL_HPP
