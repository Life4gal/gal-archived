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

	namespace constant
	{
		constexpr int multiple_return = -1;
		constexpr index_type registry_index = -10000;
		constexpr index_type environment_index = -10001;
		constexpr index_type global_safe_index = -1002;
	}// namespace constant

	[[nodiscard]] constexpr bool is_upvalue_index(const index_type i) noexcept { return i < constant::global_safe_index; }

	[[nodiscard]] constexpr auto get_upvalue_index(const index_type i) noexcept { return constant::global_safe_index - i; }

	[[nodiscard]] constexpr bool is_pseudo(const index_type i) noexcept { return i <= constant::registry_index; }

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
		GAL_API [[nodiscard]] main_state* new_state();
		GAL_API void destroy_state(main_state& state);

		GAL_API [[nodiscard]] child_state* new_thread(main_state& state);
		GAL_API [[nodiscard]] main_state& main_thread(const child_state& state);

		GAL_API void reset_thread(child_state& state);
		GAL_API [[nodiscard]] boolean_type is_thread_reset(const child_state& state);
	}// namespace state

	/**
	 * @brief basic stack manipulation
	 */
	namespace stack
	{
		GAL_API [[nodiscard]] index_type abs_index(const child_state& state, index_type index) noexcept;

		GAL_API [[nodiscard]] index_type get_top(const child_state& state) noexcept;
		GAL_API void set_top(child_state& state, index_type index) noexcept;

		GAL_API void push(child_state& state, index_type index) noexcept;
		GAL_API void remove(child_state& state, index_type index) noexcept;
		GAL_API void insert(child_state& state, index_type index) noexcept;
		GAL_API void replace(child_state& state, index_type index) noexcept;

		GAL_API boolean_type check(child_state& state, stack_size_type size) noexcept;
		/**
		 * @brief Allows for unlimited stack frames
		 */
		GAL_API void raw_check(child_state& state, stack_size_type size);

		GAL_API void exchange_move(child_state& from, child_state& to, stack_size_type num);
		GAL_API void exchange_push(const child_state& from, child_state& to, index_type index);
	}// namespace stack

	/**
	 * @brief Access functions (stack -> C++) / Push functions (C++ -> stack)
	 */
	namespace internal
	{
		GAL_API [[nodiscard]] boolean_type is_number(const child_state& state, index_type index) noexcept;
		GAL_API [[nodiscard]] boolean_type	is_string(const child_state& state, index_type index) noexcept;
		GAL_API [[nodiscard]] boolean_type	is_internal_function(const child_state& state, index_type index) noexcept;
		GAL_API [[nodiscard]] boolean_type	is_gal_function(const child_state& state, index_type index) noexcept;
		GAL_API [[nodiscard]] boolean_type	is_user_data(const child_state& state, index_type index) noexcept;

		GAL_API [[nodiscard]] object_type	get_type(const child_state& state, index_type index) noexcept;
		GAL_API [[nodiscard]] string_type	get_typename(object_type type) noexcept;
		GAL_API [[nodiscard]] unsigned_type get_object_length(const child_state& state, index_type index);

		GAL_API [[nodiscard]] boolean_type is_equal(const child_state& state, index_type index1, index_type index2);
		GAL_API [[nodiscard]] boolean_type	is_raw_equal(const child_state& state, index_type index1, index_type index2) noexcept;
		GAL_API [[nodiscard]] boolean_type is_less_than(child_state& state, index_type index1, index_type index2);

		GAL_API [[nodiscard]] boolean_type	to_boolean(const child_state& state, index_type index) noexcept;
		GAL_API [[nodiscard]] number_type	to_number(const child_state& state, index_type index, boolean_type* converted) noexcept;
		GAL_API [[nodiscard]] string_type	to_string(child_state& state, index_type index, size_t* length);
		GAL_API [[nodiscard]] string_type to_string_atomic(const child_state& state, index_type index, int* atomic) noexcept;
		GAL_API [[nodiscard]] string_type to_named_call_atomic(const child_state& state, int* atomic) noexcept;
		GAL_API [[nodiscard]] internal_function_type to_internal_function(const child_state& state, index_type index) noexcept;
		GAL_API [[nodiscard]] child_state*			 to_thread(const child_state& state, index_type index) noexcept;
		GAL_API [[nodiscard]] const void*			 to_pointer(const child_state& state, index_type index) noexcept;

		GAL_API [[nodiscard]] user_data_type		 to_user_data(const child_state& state, index_type index) noexcept;
		GAL_API user_data_type to_user_data_tagged(child_state& state, index_type index, user_data_tag_type tag);
		GAL_API user_data_tag_type get_user_data_tag(child_state& state, index_type index);

		GAL_API void push_null(child_state& state);
		GAL_API void push_boolean(child_state& state, boolean_type boolean);
		GAL_API void push_number(child_state& state, number_type number);
		GAL_API void push_integer(child_state& state, integer_type integer);
		GAL_API void push_unsigned(child_state& state, unsigned_type u);
		GAL_API void push_string_sized(child_state& state, string_type string, size_t length);
		GAL_API void push_string(child_state& state, string_type string);
		GAL_API void push_closure(child_state& state, internal_function_type function, string_type debug_name, unsigned_type num_params, continuation_function_type continuation);
		GAL_API void push_light_user_data(child_state& state, user_data_type user_data);
		GAL_API boolean_type push_thread(child_state& state);
	}// namespace internal

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
