#pragma once

#ifndef GAL_LANG_VM_GAL_HPP
#define GAL_LANG_VM_GAL_HPP

#ifndef GAL_API
#define GAL_API extern
#endif

#define GAL_LIB_API GAL_API

#include <config/config.hpp>

namespace gal::vm_bak
{
	using user_data_type = void*;

	using null_type = ast::gal_null_type;
	using boolean_type = ast::gal_boolean_type;
	using number_type = ast::gal_number_type;
	using integer_type = int;
	using unsigned_type = unsigned int;

	using index_type = integer_type;
	using stack_size_type = index_type;

	using vector_element_type = float;
	using vector_type = const vector_element_type*;//[4];
	using string_type = const char*;

	namespace constant
	{
		constexpr int multiple_return = -1;
		constexpr index_type registry_index = -10000;
		constexpr index_type environ_index = -10001;
		constexpr index_type global_safe_index = -1002;
	}

	constexpr auto get_upvalue_index(const index_type i) noexcept { return constant::global_safe_index - i; }

	constexpr bool is_pseudo(const index_type i) noexcept { return i <= constant::registry_index; }

	enum class vm_status
	{
		ok = 0,
		yield,
		error_run,
		error_syntax,
		error_memory,
		error_error,
		breakpoint,
		// yielded for a debug breakpoint
	};

	constexpr std::uint8_t unknown_object_type(-1);
	enum class object_type : std::uint8_t
	{
		null = 0,
		boolean = 1,

		light_user_data,
		number,
		vector,

		// all types above this must be value types, all types below this must be GC types
		string,

		table,
		function,
		user_data,
		thread,

		// values below this line are used in gc_object tags but may never show up in gal_value type tags
		proto,
		upvalue,
		dead_key,

		// the count of gal_value type tags
		tagged_value_count = proto
	};

	enum class gc_operand
	{
		stop,
		restart,
		collect,
		count,
		count_byte,
		running,

		// garbage collection is handled by 'assists' that perform some amount of GC work matching pace of allocation
		// explicit GC steps allow to perform some amount of work at custom points to offset the need for GC assists
		// note that GC might also be paused for some duration (until bytes allocated meet the threshold)
		// if an explicit step is performed during this pause, it will trigger the start of the next collection cycle
		step,

		set_goal,
		set_step_multiple,
		set_step_size
	};

	class thread_state;

	using internal_function_type = index_type(*)(thread_state* state);
	using continuation_type = index_type(*)(thread_state* state, vm_status status);

	/**
	 * @brief state manipulation
	 */
	namespace state
	{
		GAL_API thread_state* new_state(user_data_type user_data);
		GAL_API void destroy_state(thread_state& state);

		GAL_API thread_state* new_thread(thread_state& state);
		GAL_API thread_state* main_thread(thread_state& state);

		GAL_API void reset_thread(thread_state& state);
		GAL_API boolean_type is_thread_reset(thread_state& state);
	}

	/**
	 * @brief basic stack manipulation
	 */
	namespace stack
	{
		GAL_API index_type abs_index(thread_state& state, index_type index);

		GAL_API index_type get_top(thread_state& state);
		GAL_API void set_top(thread_state& state, index_type index);

		GAL_API void push(thread_state& state, index_type index);
		GAL_API void remove(thread_state& state, index_type index);
		GAL_API void insert(thread_state& state, index_type index);
		GAL_API void replace(thread_state& state, index_type index);

		GAL_API boolean_type check(thread_state& state, stack_size_type size);
		/**
		 * @brief Allows for unlimited stack frames
		 */
		GAL_API void raw_check(thread_state& state, stack_size_type size);

		GAL_API void exchange_move(thread_state& from, thread_state& to, stack_size_type num);
		GAL_API void exchange_push(thread_state& from, thread_state& to, index_type index);
	}

	/**
	 * @brief Access functions (stack -> C++) / Push functions (C++ -> stack)
	 */
	namespace internal
	{
		GAL_API boolean_type is_number(thread_state& state, index_type index);
		GAL_API boolean_type is_string(thread_state& state, index_type index);
		GAL_API boolean_type is_internal_function(thread_state& state, index_type index);
		GAL_API boolean_type is_gal_function(thread_state& state, index_type index);
		GAL_API boolean_type is_user_data(thread_state& state, index_type index);

		GAL_API object_type get_type(thread_state& state, index_type index);
		GAL_API string_type get_typename(thread_state& state, index_type index);
		GAL_API unsigned_type get_object_length(thread_state& state, index_type index);

		GAL_API boolean_type is_equal(thread_state& state, index_type index1, index_type index2);
		GAL_API boolean_type is_raw_equal(thread_state& state, index_type index1, index_type index2);
		GAL_API boolean_type is_less_than(thread_state& state, index_type index1, index_type index2);

		GAL_API boolean_type to_boolean(thread_state& state, index_type index);
		GAL_API number_type to_number(thread_state& state, index_type index, boolean_type* converted);
		GAL_API integer_type to_integer(thread_state& state, index_type index, boolean_type* converted);
		GAL_API unsigned_type to_unsigned(thread_state& state, index_type index, boolean_type* converted);
		GAL_API vector_type to_vector(thread_state& state, index_type index);
		GAL_API string_type to_string(thread_state& state, index_type index, size_t* length);
		GAL_API string_type to_string_atomic(thread_state& state, index_type index, int* atomic);
		GAL_API string_type to_named_call_atomic(thread_state& state, int* atomic);
		GAL_API internal_function_type to_internal_function(thread_state& state, index_type index);
		GAL_API thread_state* to_thread(thread_state& state, index_type index);
		GAL_API const void* to_pointer(thread_state& state, index_type index);

		GAL_API user_data_type to_user_data(thread_state& state, index_type index);
		GAL_API user_data_type to_user_data_tagged(thread_state& state, index_type index, user_data_tag_type tag);
		GAL_API user_data_tag_type get_user_data_tag(thread_state& state, index_type index);

		GAL_API void push_null(thread_state& state);
		GAL_API void push_boolean(thread_state& state, boolean_type boolean);
		GAL_API void push_number(thread_state& state, number_type number);
		GAL_API void push_integer(thread_state& state, integer_type integer);
		GAL_API void push_unsigned(thread_state& state, unsigned_type u);
		GAL_API void push_vector3(thread_state& state, vector_element_type x, vector_element_type y, vector_element_type z);
		GAL_API void push_vector4(thread_state& state, vector_element_type x, vector_element_type y, vector_element_type z, vector_element_type w);
		GAL_API void push_string_sized(thread_state& state, string_type string, size_t length);
		GAL_API void push_string(thread_state& state, string_type string);
		GAL_API void push_closure(thread_state& state, internal_function_type function, string_type debug_name, unsigned_type num_params, continuation_type continuation);
		GAL_API void push_light_user_data(thread_state& state, user_data_type user_data);
		GAL_API boolean_type push_thread(thread_state& state);
	}

	/**
	 * @brief Get functions (GAL -> stack) / Set functions (stack -> GAL)
	 */
	namespace interface
	{
		GAL_API void get_table(thread_state& state, index_type index);
		GAL_API void get_field(thread_state& state, index_type index, string_type key);
		GAL_API void raw_get_field(thread_state& state, index_type index, string_type key);
		GAL_API void raw_get(thread_state& state, index_type index);
		GAL_API void raw_get_integer(thread_state& state, index_type index, integer_type n);
		GAL_API void create_table(thread_state& state, unsigned_type array_size, unsigned_type list_size);

		GAL_API void set_mutable(thread_state& state, index_type index, boolean_type m);
		GAL_API boolean_type get_mutable(thread_state& state, index_type index);
		GAL_API void set_sharable(thread_state& state, index_type index, boolean_type sharable);

		GAL_API user_data_type new_user_data_tagged(thread_state& state, size_t size, user_data_tag_type tag);
		GAL_API user_data_type new_user_data_with_destructor(thread_state& state, size_t size, void (*destructor)(user_data_type));
		GAL_API boolean_type get_meta_table(thread_state& state, index_type index);
		GAL_API void get_function_environment(thread_state& state, index_type index);

		GAL_API void set_table(thread_state& state, index_type index);
		GAL_API void set_field(thread_state& state, index_type index, string_type key);
		GAL_API void raw_set(thread_state& state, index_type index);
		GAL_API void raw_set_integer(thread_state& state, index_type index, integer_type n);
		GAL_API boolean_type set_meta_table(thread_state& state, index_type index);
		GAL_API boolean_type set_function_environment(thread_state& state, index_type index);
	}

	namespace bytecode
	{
		GAL_API boolean_type load(thread_state& state, string_type chunk_name, const char* data, size_t size, int environment);
		GAL_API void call(thread_state& state, unsigned_type num_args, unsigned_type num_returns);
		GAL_API void call_with_handler(thread_state& state, unsigned_type num_args, unsigned_type num_returns, index_type error_handler);
	}

	namespace coroutine
	{
		GAL_API integer_type thread_yield(thread_state& state, unsigned_type num_returns);
		GAL_API integer_type thread_break(thread_state& state);
		GAL_API integer_type thread_resume(thread_state& state, thread_state& from, unsigned_type num_args);
		GAL_API integer_type thread_resume_error(thread_state& state, thread_state& from);
		GAL_API integer_type thread_status(thread_state& state);
		GAL_API boolean_type thread_can_yield(thread_state& state);

		GAL_API user_data_type get_thread_user_data(thread_state& state);
		GAL_API user_data_type set_thread_user_data(thread_state& state, user_data_type user_data);
	}

	namespace memory
	{
		GAL_API integer_type gc(thread_state& state, gc_operand operand, integer_type data);
	}

	namespace utility
	{
		GAL_API inline number_type to_number(thread_state& state, const index_type index) { return internal::to_number(state, index, nullptr); }

		GAL_API inline integer_type to_integer(thread_state& state, const index_type index) { return internal::to_integer(state, index, nullptr); }

		GAL_API inline unsigned_type to_unsigned(thread_state& state, const index_type index) { return internal::to_unsigned(state, index, nullptr); }

		GAL_API inline void pop(thread_state& state, const index_type n) { stack::set_top(state, -n - 1); }

		GAL_API inline void new_table(thread_state& state) { interface::create_table(state, 0, 0); }

		GAL_API inline unsigned_type string_length(thread_state& state, const index_type index) { return internal::get_object_length(state, index); }

		GAL_API inline boolean_type is_unknown(thread_state& state, const index_type index) { return static_cast<std::uint8_t>(internal::get_type(state, index)) == unknown_object_type; }

		GAL_API inline boolean_type is_null(thread_state& state, const index_type index) { return internal::get_type(state, index) == object_type::null; }

		GAL_API inline boolean_type is_unknown_or_null(thread_state& state, const index_type index) { return is_unknown(state, index) || is_null(state, index); }

		GAL_API inline boolean_type is_boolean(thread_state& state, const index_type index) { return internal::get_type(state, index) == object_type::boolean; }

		GAL_API inline boolean_type is_light_user_data(thread_state& state, const index_type index) { return internal::get_type(state, index) == object_type::light_user_data; }

		GAL_API inline boolean_type is_number(thread_state& state, const index_type index) { return internal::get_type(state, index) == object_type::number; }

		GAL_API inline boolean_type is_vector(thread_state& state, const index_type index) { return internal::get_type(state, index) == object_type::vector; }

		GAL_API inline boolean_type is_table(thread_state& state, const index_type index) { return internal::get_type(state, index) == object_type::table; }

		GAL_API inline boolean_type is_function(thread_state& state, const index_type index) { return internal::get_type(state, index) == object_type::function; }

		GAL_API inline boolean_type is_thread(thread_state& state, const index_type index) { return internal::get_type(state, index) == object_type::thread; }

		GAL_API inline string_type to_string(thread_state& state, const index_type index) { return internal::to_string(state, index, nullptr); }

		template<size_t N>
		GAL_API void push_literal(thread_state& state, const char (&string)[N]) { internal::push_string_sized(state, string, N); }

		GAL_API inline void push_function(thread_state& state, const internal_function_type function, const string_type debug_name) { internal::push_closure(state, function, debug_name, 0, nullptr); }

		GAL_API inline void push_closure(thread_state& state, const internal_function_type function, const string_type debug_name, unsigned_type num_params) { internal::push_closure(state, function, debug_name, num_params, nullptr); }

		GAL_API inline void set_global(thread_state& state, const string_type key) { interface::set_field(state, constant::global_safe_index, key); }

		GAL_API inline void get_global(thread_state& state, const string_type key) { interface::get_field(state, constant::global_safe_index, key); }
	}

	namespace debug
	{
		struct gal_debug
		{
			string_type name;
			string_type what;
			string_type source;
			int			line_defined;
			int			current_line;
			compiler::operand_abc_underlying_type num_upvalues;
			compiler::operand_abc_underlying_type num_params;
			boolean_type						  is_vararg;
			char								  short_source[max_id_size];
			user_data_type						  user_data;
		};

		// Functions to be called by the debugger in specific events
		using gal_hook = void(*)(thread_state& state, gal_debug* ar);
		using gal_coverage = void(*)(user_data_type context, string_type function, int line_defined, int depth, const boolean_type* hits, size_t size);

		GAL_API boolean_type get_info(thread_state& state, int level, string_type what, gal_debug* ar);
		GAL_API boolean_type get_argument(thread_state& state, int level, compiler::operand_abc_underlying_type num);
		GAL_API string_type	 get_local(thread_state& state, int level, compiler::operand_abc_underlying_type num);
		GAL_API string_type	 set_local(thread_state& state, int level, compiler::operand_abc_underlying_type num);
		GAL_API string_type	 get_upvalue(thread_state& state, index_type index, compiler::operand_abc_underlying_type num);
		GAL_API string_type	 set_upvalue(thread_state& state, index_type index, compiler::operand_abc_underlying_type mum);

		GAL_API void	 single_step(thread_state& state, boolean_type enabled);
		GAL_API void		 breakpoint(thread_state& state, index_type index, int line, boolean_type enabled);

		GAL_API void		 get_coverage(thread_state& state, index_type index, user_data_type context, gal_coverage callback);

		// this function is not thread-safe since it stores the result in a shared global array! Only use for debugging
		GAL_API string_type	 debug_trace(thread_state& state);
	}

	namespace coroutine
	{
		struct gal_callback
		{
			// arbitrary user_data pointer that is never overwritten by GAL
			user_data_type user_data;

			// gets called at safe_points (loop back edges, call/ret, gc) if set
			void (*interrupt)(thread_state& state, int gc);
			// gets called when an unprotected error is raised
			void (*panic)(thread_state& state, int error_code);

			// gets called when state is created (parent_state == parent) or destroyed (parent_state == nullptr)
			void (*user_thread)(thread_state* parent_state, thread_state& state);
			// gets called when a string is created; returned atom can be retrieved via to_string_atomic
			int16_t (*user_atomic)(string_type string, size_t length);

			// gets called when BREAK instruction is encountered
			void (*debug_break)(thread_state& state, debug::gal_debug* ar);
			// gets called after each instruction in single step mode
			void (*debug_step)(thread_state& state, debug::gal_debug* ar);
			// gets called when thread execution is interrupted by break in another thread
			void (*debug_interrupt)(thread_state& state, debug::gal_debug* ar);
			// gets called when handled call results in an error
			void (*debug_handled_error)(thread_state& state);
		};

		GAL_API gal_callback* callback(thread_state& state);
	}
}

#endif // GAL_LANG_VM_GAL_HPP
