#pragma once

#ifndef GAL_LANG_VM_HPP
	#define GAL_LANG_VM_HPP

	#include <allocator.hpp>
	#include <forward_list>
	#include <gal.hpp>
	#include <stack>
	#include <vm/compiler.hpp>
#include <vm/value.hpp>

namespace gal
{
	enum class opcodes_type : std::uint8_t
	{
	#define OPCODE(name, _) CODE_##name
	#include <vm/opcodes.config>
	#undef OPCODE
	};

	inline std::uint8_t code_to_scalar(opcodes_type code)
	{
		return static_cast<std::uint8_t>(code);
	}

	/**
	 * @brief A handle to a value.
	 *
	 * Note that even non-heap-allocated values can be stored here.
	 */
	struct gal_handle
	{
		magic_value value;
	};

	class gal_virtual_machine_state
	{
	public:
		friend class gal_virtual_machine;

		constexpr static magic_value							 core_module_key = magic_value_null;
		constexpr static const char								 core_module_name[] = "global";
		constexpr static auto									 core_module_name_length = sizeof(core_module_name) - 1;

		/**
		 * @brief The fiber that is currently running.
		 */
		object_fiber*											 fiber_{nullptr};

		/**
		 * @brief The loaded modules. Each key is an object_string (except for
		 * the main module, whose key is magic_value_null) for the module's
		 * name and the value is the object_module for the module.
		 */
		object_map												 modules_;

		/**
		 * @brief The most recently imported module. More specifically, the module whose
		 * code has most recently finished executing.
		 *
		 * Not treated like a GC root since the module is already in [modules].
		 */
		object_module*											 last_module_{nullptr};

		// Memory management data below:
		// vvv

		/**
		 * @brief The number of bytes that are known to be currently allocated. Includes all
		 * memory that was proven live after the last GC, as well as any new bytes
		 * that were allocated since then. Does *not* include bytes for objects that
		 * were freed since the last GC.
		 */
		gal_size_type											 bytes_allocated_{0};

		/**
		 * @brief The number of total allocated bytes that will trigger the next GC.
		 */
		gal_size_type											 next_gc_;

		/**
		 * @brief The linked list of all currently allocated objects.
		 */
		std::forward_list<object*, gal_allocator<object*>>		 objects_;

		/**
		 * @brief The linked list of active handles.
		 */
		std::forward_list<gal_handle, gal_allocator<gal_handle>> handles_;

		/**
		 * @brief Pointer to the bottom of the range of stack slots available for use from
		 * the C++ API. During a outer method, this will be in the stack of the fiber
		 * that is executing a method.
		 *
		 * If not in a outer method, this is initially nullptr. If the user requests
		 * slots by calling gal_ensure_slots(), a stack is created and this is
		 * initialized.
		 */
		magic_value*											 api_stack_{nullptr};

		gal_configuration										 configuration_;

		// Compiler and debugger data below:
		// vvv

		/**
		 * @brief The compiler that is currently compiling code. This is used so that heap
		 * allocated objects used by the compiler can be found if a GC is kicked off
		 * in the middle of a compile.
		 */
		gal_compiler*												 compiler_{nullptr};

		/**
		 * @brief There is a single global symbol table for all method names on all classes.
		 * Method calls are dispatched directly by index in this table.
		 */
		symbol_table											 method_names_;

		explicit gal_virtual_machine_state(gal_configuration configuration)
			: 
			  next_gc_(configuration.min_heap_size),
			  configuration_(configuration) {}

		[[nodiscard]] gal_size_type get_slot_count() const noexcept
		{
			if (api_stack_)
			{
				return fiber_->get_current_stack_size(api_stack_);
			}

			return 0;
		}

		[[nodiscard]] const magic_value* get_stack_bottom() const noexcept
		{
			return api_stack_;
		}

		void set_stack_bottom(magic_value* new_bottom) noexcept
		{
			api_stack_ = new_bottom;
		}

		void shutdown_stack() noexcept
		{
			api_stack_ = nullptr;
		}

		void					  validate_slot(gal_slot_type slot) const;

		void					  ensure_slot(gal_slot_type slots);

		[[nodiscard]] magic_value get_slot_value(gal_slot_type slot) const noexcept
		{
			validate_slot(slot);
			return api_stack_[slot];
		}

		void set_slot_value(gal_slot_type slot, magic_value value) const noexcept
		{
			// todo: release existing object
			validate_slot(slot);
			api_stack_[slot] = value;
		}

		[[nodiscard]] gal_object_type get_slot_type(gal_slot_type slot) const;

		/**
		 * @brief Invoke the finalizer for the outer object referenced by [outer].
		 */
		void						  finalize_outer(object_outer& outer) const;

		/**
		 * @brief Creates a new [gal_handle] for [value].
		 */
		gal_handle*					  make_handle(magic_value value);

		/**
		 * @brief Compile [source] in the context of [module] and wrap in a fiber that can
		 * execute it.
		 *
		 * Returns nullptr if a compile error occurred.
		 */
		object_closure*				  compile_source(const char* module, const char* source, bool is_expression, bool print_errors);

		/**
		  * @brief Looks up a variable from a previously-loaded module.
		  *
		  * Aborts the current fiber if the module or variable could not be found.
		  */
		magic_value					  get_module_variable(magic_value module_name, const object_string& variable_name);

		magic_value					  get_module_variable(magic_value module_name, object_string::const_pointer variable_name);

		/**
		 * @brief Create an object on the heap and add it to the linked list.
		 *
		 * The reason why this function is needed is to avoid explicitly allocating objects
		 * with new. Here we can use the specified allocator to get the object.
		 *
		 * note: All memory used by members of the class is managed by itself (usually STL components), and we only manage the class object itself
		 */
		template<typename T, typename... Args>
		requires std::is_base_of_v<object, T>
		auto make_object(Args&&... args)
		{
			return dynamic_cast<T*>(objects_.emplace_front(object::create<T>(std::forward<Args>(args)...)));
		}

		void		bind_outer_class(object_class& obj_class, object_module& module);

	private:
		void						   init_core_module();

		/**
		 * @brief Looks up a outer method in [module_name] on [class_name] with [signature].
		 *
		 * This will try the host's outer method binder first. If that fails, it
		 * falls back to handling the built-in modules.
		 */
		gal_outer_method_function_type find_outer_method(const char* module_name, const char* class_name, bool is_static, const char* signature);

		/**
		 * @brief Defines [method_value] as a method on [obj_class].
		 *
		 * Handles both outer methods where [method_value] is a string containing the
		 * method's signature and GAL methods where [method_value] is a function.
		 *
		 * Aborts the current fiber if the method is a outer method that could not be
		 * found.
		 */
		void						   bind_method(opcodes_type method_type, gal_index_type symbol, const object_module& module, object_class& obj_class, magic_value method_value);

		void						   call_outer(object_fiber& fiber, gal_outer_method_function_type outer, gal_size_type num_args);

		/**
		 * @brief Handles the current fiber having aborted because of an error.
		 *
		 * Walks the call chain of fibers, aborting each one until it hits a fiber that
		 * handles the error. If none do, tells the VM to stop.
		 */
		void						   runtime_error();

		/**
		 * @brief Aborts the current fiber with an appropriate method not found error for a
		 * method with [symbol] on [obj_class].
		 */
		void						   method_not_found(object_class& obj_class, gal_index_type symbol);

		/**
		 * @brief Looks up the previously loaded module with [name].
		 *
		 * Returns `nullptr` if no module with that name has been loaded.
		 */
		[[nodiscard]] object_module*   get_module(magic_value name) const;

		object_closure*				   compile_in_module(magic_value name, const char* source, bool is_expression, bool print_errors);

		void						   create_outer(magic_value* stack);

		/**
		 * @brief Let the host resolve an imported module name if it wants to.
		 */
		object_string				   resolve_module(const object_string& name);

		magic_value					   import_module(const object_string& name);

		magic_value					   get_module_variable(object_module& module, const object_string& variable_name);

		bool						   check_arity(magic_value value, gal_size_type num_args) const;

		/**
		 * @brief The main byte-code interpreter loop. This is where the magic happens. It is
		 * also, as you can imagine, highly performance critical.
		 */
		gal_interpret_result		   run_interpreter(object_fiber* fiber);

		/**
		 * @brief Validate everything here below.
		 */

		bool						   validate_helper(const char* arg_name, const char* requires_type);

	public:
		/**
		 * @brief Validates that the given [arg] is a function. Returns true if it is. If not,
		 * reports an error and returns false.
		 */
		bool		   validate_function(magic_value arg, const char* arg_name);

		/**
		 * @brief Validates that the given [arg] is a Num. Returns true if it is. If not,
		 * reports an error and returns false.
		 */
		bool		   validate_number(magic_value arg, const char* arg_name);

		/**
		 * @brief Validates that [value] is an integer. Returns true if it is. If not, reports
		 * an error and returns false.
		 */
		bool		   validate_int_value(double value, const char* arg_name);

		/**
		 * @brief Validates that the given [arg] is an integer. Returns true if it is. If not,
		 * reports an error and returns false.
		 */
		bool		   validate_int(magic_value arg, const char* arg_name);

		/**
		 * @brief Validates that [arg] is a valid object for use as a map key. Returns true if
		 * it is. If not, reports an error and returns false.
		 */
		bool		   validate_key(magic_value arg);

		/**
		 * @brief Validates that [value] is an integer within `[0, count)`. Also allows
		 * negative indices which map backwards from the end. Returns the valid positive
		 * index value. If invalid, reports an error and returns `gal_index_not_exist`.
		 */
		gal_index_type validate_index_value(double value, gal_size_type count, const char* arg_name);

		/**
		 * @brief Validates that the argument at [arg] is an integer within `[0, count)`.
		 * Also allows negative indices which map backwards from the end. Returns the
		 * valid positive index value. If invalid, reports an error and returns `gal_index_not_exist`.
		 */
		gal_index_type validate_index(magic_value arg, gal_size_type count, const char* arg_name);

		/**
		 * @brief Validates that the given [arg] is a String. Returns true if it is. If not,
		 * reports an error and returns false.
		 */
		bool		   validate_string(magic_value arg, const char* arg_name);
	};
}// namespace gal

#endif//GAL_LANG_VM_HPP
