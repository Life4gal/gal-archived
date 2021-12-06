#pragma once

#ifndef GAL_LANG_VM_HPP
	#define GAL_LANG_VM_HPP

	#include <allocator.hpp>
	#include <array>
	#include <forward_list>
	#include <gal.hpp>
	#include <stack>
	#include <memory>
	#include <vm/common.hpp>
	#include <vm/compiler.hpp>
	#include <vm/value.hpp>

namespace gal
{
	enum class opcodes_type : std::uint8_t
	{
	#include <vm/opcodes.config>
	};

	std::uint8_t code_to_scalar(opcodes_type code)
	{
		return static_cast<std::uint8_t>(code);
	}

	/**
	 * @brief A handle to a value, basically just a linked list of extra GC roots.
	 *
	 * Note that even non-heap-allocated values can be stored here.
	 */
	struct gal_handle
	{
		magic_value value{};

		gal_handle* prev{nullptr};
		gal_handle* next{nullptr};
	};

	class gal_virtual_machine_state
	{
	public:
		friend class gal_virtual_machine;

		constexpr static gal_index_type variable_already_defined	 = -1;
		constexpr static gal_index_type variable_too_many_defined	 = -2;
		constexpr static gal_index_type variable_used_before_defined = -3;

	#ifndef GAL_MAX_TEMP_ROOTS
		constexpr static gal_size_type max_temp_roots = 8;
	#else
		constexpr static gal_size_type max_temp_roots = GAL_MAX_TEMP_ROOTS;
	#endif

		std::shared_ptr<object_class>									  boolean_class_;
		std::shared_ptr<object_class>									  class_class_;
		std::shared_ptr<object_class>									  fiber_class_;
		std::shared_ptr<object_class>									  function_class_;
		std::shared_ptr<object_class>									  list_class_;
		std::shared_ptr<object_class>									  map_class_;
		std::shared_ptr<object_class>									  null_class_;
		std::shared_ptr<object_class>									  number_class_;
		std::shared_ptr<object_class>									  object_class_;
		std::shared_ptr<object_class>									  range_class_;
		std::shared_ptr<object_class>									  string_class_;

		/**
		 * @brief The fiber that is currently running.
		 */
		object_fiber*													  fiber_;

		/**
		 * @brief The loaded modules. Each key is an object_string (except for
		 * the main module, whose key is null) for the module's name and the
		 * value is the object_module for the module.
		 *
		 * maybe we need a global module?
		 */
		object_map														  modules_;

		/**
		 * @brief The most recently imported module. More specifically, the module whose
		 * code has most recently finished executing.
		 *
		 * Not treated like a GC root since the module is already in [modules].
		 */
		object_module*													  last_module_;

		// Memory management data below:
		// vvv

		/**
		 * @brief The number of bytes that are known to be currently allocated. Includes all
		 * memory that was proven live after the last GC, as well as any new bytes
		 * that were allocated since then. Does *not* include bytes for objects that
		 * were freed since the last GC.
		 */
		gal_size_type													  bytes_allocated_;

		/**
		 * @brief The number of total allocated bytes that will trigger the next GC.
		 */
		gal_size_type													  next_gc_;

		/**
		 * @brief The linked list of all currently allocated objects.
		 */
		std::forward_list<object*, gal_allocator<object*>>				  objects_;

		/**
		 * @brief The "gray" set for the garbage collector. This is the stack of unprocessed
		 * objects while a garbage collection pass is in process.
		 */
		std::stack<object*, std::vector<object*, gal_allocator<object*>>> gray_;

		/**
		 * @brief The list of temporary roots. This is for temporary or new objects that are
		 * not otherwise reachable but should not be collected.
		 *
		 * They are organized as a stack of pointers stored in this array. This
		 * implies that temporary roots need to have stack semantics: only the most
		 * recently pushed object can be released.
		 */
		std::array<object*, max_temp_roots>								  temp_roots_;
		std::array<object*, max_temp_roots>::size_type					  num_temp_roots_;

		/**
		 * @brief Pointer to the first node in the linked list of active handles or nullptr if
		 * there are none.
		 */
		gal_handle*														  handles_;

		/**
		 * @brief Pointer to the bottom of the range of stack slots available for use from
		 * the C++ API. During a outer method, this will be in the stack of the fiber
		 * that is executing a method.
		 *
		 * If not in a outer method, this is initially nullptr. If the user requests
		 * slots by calling gal_ensure_slots(), a stack is created and this is
		 * initialized.
		 */
		magic_value*													  api_stack_;

		gal_configuration												  configuration;

		// Compiler and debugger data below:
		// vvv

		/**
		 * @brief The compiler that is currently compiling code. This is used so that heap
		 * allocated objects used by the compiler can be found if a GC is kicked off
		 * in the middle of a compile.
		 */
		compiler*														  compiler_;

		/**
		 * @brief There is a single global symbol table for all method names on all classes.
		 * Method calls are dispatched directly by index in this table.
		 */
		symbol_table													  method_names_;

		[[nodiscard]] gal_size_type										  get_slot_count() const
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

		void					  validate_slot(gal_slot_type slot) const;

		void					  ensure_slot(gal_slot_type slots);

		[[nodiscard]] magic_value get_slot_value(gal_slot_type slot) const
		{
			validate_slot(slot);
			return api_stack_[slot];
		}

		void set_slot_value(gal_slot_type slot, magic_value value)
		{
			validate_slot(slot);
			api_stack_[slot] = value;
		}

		[[nodiscard]] gal_object_type get_slot_type(gal_slot_type slot) const;

		/**
		 * @brief get the real index if `size_type` and `index_type` has different type
		 *
		 * Bounded:
		 *      [1,    2,    3,    4,    5]
		 *       ^0    ^1    ^2    ^3    ^4
		 *       ^-5   ^-4   ^-3   ^-2   ^-1
		 * Unbounded:
		 *      [1,    2,    3,    4,    5]    [insertable position here]
		 *       ^0    ^1    ^2    ^3    ^4    ^5
		 *       ^-6   ^-5   ^-4   ^-3   ^-2   ^-1
		 */
		template<bool Bounded>
		[[nodiscard]] static gal_size_type validate_index(gal_size_type target_size, gal_index_type index) noexcept
		{
			auto ret = static_cast<gal_size_type>(index);
			if (std::cmp_greater(ret, index))
			{
				// negative index
				if constexpr (Bounded)
				{
					ret = target_size - (std::numeric_limits<gal_size_type>::max() - ret + 1);
				}
				else
				{
					ret = target_size - (std::numeric_limits<gal_size_type>::max() - ret + 1) + 1;
				}
			}
			return ret;
		}

		/**
		 * @brief Invoke the finalizer for the outer object referenced by [outer].
		 */
		void			   finalize_outer(object_outer& outer) const;

		/**
		 * @brief Creates a new [gal_handle] for [value].
		 */
		gal_handle*		   make_handle(magic_value value);

		/**
		 * @brief Compile [source] in the context of [module] and wrap in a fiber that can
		 * execute it.
		 *
		 * Returns nullptr if a compile error occurred.
		 */
		object_closure*	   compile_source(const char* module, const char* source, bool is_expression, bool print_errors);

		/**
		  * @brief Looks up a variable from a previously-loaded module.
		  *
		  * Aborts the current fiber if the module or variable could not be found.
		  */
		magic_value		   get_module_variable(magic_value module_name, magic_value variable_name);

		/**
		  * @brief Returns the value of the module-level variable named [name] in the main
		  * module.
		  */
		static magic_value find_variable(object_module& module, const char* name);

		/**
		  * @brief Adds a new implicitly declared top-level variable named [name] to [module]
		  * based on a use site occurring on [line].
		  *
		  * Does not check to see if a variable with that name is already declared or
		  * defined. Returns the symbol for the new variable or `variable_too_many_defined`
		  * if there are too many variables defined.
		  */
		gal_index_type	   declare_variable(object_module& module, const char* name, gal_size_type length, gal_size_type line);

		/**
		  * @brief Adds a new top-level variable named [name] to [module], and optionally
		  * populates line with the line of the implicit first use (line can be nullptr).
		  *
		  * Returns the symbol for the new variable, `variable_already_defined` if a variable
		  * with the given name is already defined, or `variable_too_many_defined` if there
		  * are too many variables defined. Returns `variable_used_before_defined` if this is
		  * a top-level lowercase variable (local name) that was used before being defined.
		  */
		gal_index_type	   define_variable(object_module& module, const char* name, gal_size_type length, magic_value value, int* line = nullptr);

		/**
		  * @brief Adds a new top-level variable named [name] to [module], and optionally
		  * populates line with the line of the implicit first use (line can be nullptr).
		  *
		  * Returns the symbol for the new variable, `variable_already_defined` if a variable
		  * with the given name is already defined, or `variable_too_many_defined` if there
		  * are too many variables defined. Returns `variable_used_before_defined` if this is
		  * a top-level lowercase variable (local name) that was used before being defined.
		  */
		gal_index_type	   define_variable(object_module& module, const object_string& name, magic_value value, int* line = nullptr);

		/**
		 * @brief Pushes [closure] onto [fiber]'s callstack to invoke it. Expects [num_args]
		 * arguments (including the receiver) to be on the top of the stack already.
		 */
		void			   call_function(object_fiber& fiber, object_closure& closure, gal_size_type num_args)
		{
			// Grow the stack if needed.
			const auto stack_size = fiber.get_current_stack_size();
			const auto needed	  = stack_size + closure.get_function().get_slots_size();
			fiber.ensure_stack(*this, needed);

			fiber.add_call_frame(closure, *fiber.get_stack_point(num_args));
		}

		/**
		 * @brief Returns the class of [value].
		 */
		[[nodiscard]] const object_class* get_class(magic_value value) const
		{
			if (value.is_number()) { return number_class_.get(); }
			if (value.is_object()) { return value.as_object()->get_class(); }

			switch (value.get_tag())
			{
				case magic_value::tag_nan: return number_class_.get();
				case magic_value::tag_null: return null_class_.get();
				case magic_value::tag_false:
				case magic_value::tag_true: return boolean_class_.get();
				case magic_value::tag_undefined: UNREACHABLE();
			}

			UNREACHABLE();
		}

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
			return objects_.template emplace_front(object::ctor<T>(std::forward<Args>(args)...));
		}

	private:
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
		void						   bind_method(opcodes_type method_type, gal_index_type symbol, object_module& module, object_class& obj_class, magic_value method_value);

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

		/**
		 * @brief Verifies that [superclass_value] is a valid object to inherit from. That
		 * means it must be a class and cannot be the class of any built-in type.
		 *
		 * Also validates that it doesn't result in a class with too many fields and
		 * the other limitations outer classes have.
		 *
		 * If successful, returns empty object_string. Otherwise, returns a string for the runtime
		 * error message.
		 */
		object_string				   validate_superclass(const object_string& name, magic_value superclass_value, gal_size_type num_fields);

		void						   bind_outer_class(object_class& obj_class, object_module& module);

		/**
		 * @brief Completes the process for creating a new class.
		 *
		 * The class attributes instance and the class itself should be on the
		 * top of the fiber's stack.
		 *
		 * This process handles moving the attribute data for a class from
		 * compile time to runtime, since it now has all the attributes associated
		 * with a class, including for methods.
		 */
		void						   end_class() const;

		/**
		 * @brief Creates a new class.
		 *
		 * If [num_fields] is -1, the class is a outer class. The name and superclass
		 * should be on top of the fiber's stack. After calling this, the top of the
		 * stack will contain the new class.
		 *
		 * Aborts the current fiber if an error occurs.
		 */
		void						   create_class(gal_size_type num_fields, object_module* module);

		void						   create_outer(magic_value* stack);

		/**
		 * @brief Let the host resolve an imported module name if it wants to.
		 */
		magic_value					   resolve_module(magic_value name);

		magic_value					   import_module(magic_value name);

		magic_value					   get_module_variable(object_module& module, magic_value variable_name);

		bool						   check_arity(magic_value value, gal_size_type num_args);

		/**
		 * @brief The main bytecode interpreter loop. This is where the magic happens. It is
		 * also, as you can imagine, highly performance critical.
		 */
		gal_interpret_result		   run_interpreter(object_fiber* fiber);
	};
}// namespace gal

#endif//GAL_LANG_VM_HPP
