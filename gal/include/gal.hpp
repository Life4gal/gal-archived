#pragma once

#ifndef GAL_LANG_GAL_HPP
	#define GAL_LANG_GAL_HPP

	#include <cstdint>

namespace gal
{
	/**
	 * @brief A single virtual machine for executing GAL code.
	 *
	 */
	class gal_virtual_machine;
	/**
	 * @brief Virtual machine internal state
	 *
	 * GAL has no global state, so all state stored by a running interpreter lives
	 * here.
	 *
	 * Internal used only!
	 */
	class gal_virtual_machine_state;

	/**
	 * @brief A handle to a GAL object.
	 * @note This lets code outside of the VM hold a persistent reference to an object.
	 * After a handle is acquired, and until it is released, this ensures the
	 * garbage collector will not reclaim the object it references.
	 */
	struct gal_handle;

	enum class gal_error_type
	{
		// A syntax or resolution error detected at compile time.
		ERROR_COMPILE,

		// The error message for a runtime error.
		ERROR_RUNTIME,

		// One entry of a runtime error's stack trace.
		ERROR_STACK_TRACE
	};

	enum class gal_interpret_result
	{
		RESULT_SUCCESS,
		RESULT_COMPILE_ERROR,
		RESULT_RUNTIME_ERROR
	};

	/**
	 * @brief The type of object stored in a slot.
	 *
	 * This is not necessarily the object's *class*, but instead its low level
	 * representation type.
	 */
	enum class gal_object_type
	{
		BOOLEAN_TYPE,
		NUMBER_TYPE,
		OUTER_TYPE,
		LIST_TYPE,
		MAP_TYPE,
		NULL_TYPE,
		STRING_TYPE,

		// The object is of a type that isn't accessible by the C++ API.
		UNKNOWN_TYPE
	};

	using gal_row_pointer_type					 = void*;
	using gal_size_type							 = std::size_t;
	using gal_slot_type							 = std::size_t;
	using gal_index_type						 = std::uint64_t;

	constexpr gal_size_type	 gal_size_not_exist	 = -1;
	constexpr gal_slot_type	 gal_slot_not_exist	 = -1;
	constexpr gal_index_type gal_index_not_exist = -1;

	/**
	 * @brief A function callable from GAL code, but implemented in C++.
	 *
	 * Should be a gal_virtual_machine_state pointer(not reference because we maybe need convert it to void*)
	 */
	using gal_outer_method_function_type		 = void (*)(gal_virtual_machine_state*);

	/**
	 * @brief A finalizer function for freeing resources owned by an instance of a outer
	 * class. Unlike most outer methods, finalizers do not have access to the VM
	 * and should not interact with it since it's in the middle of a garbage
	 * collection.
	 */
	using gal_finalize_function_type			 = void (*)(gal_row_pointer_type);

	struct gal_configuration
	{
		struct gal_load_module_result;
		struct gal_outer_class_method;

		/**
		 * @brief Output a string of text to the user.
		 */
		using standard_output_function_type		   = void (*)(gal_virtual_machine& virtual_machine, const char* text);

		/**
		 * @brief Reports an error to the user.
		 * An error detected during compile time is reported by calling this once with
		 * [type] `ERROR_COMPILE`, the resolved name of the [module] and [line]
		 * where the error occurs, and the compiler's error [message].
		 *
		 * A runtime error is reported by calling this once with [type]
		 * `ERROR_RUNTIME`, no [module] or [line], and the runtime error's
		 * [message]. After that, a series of [type] `ERROR_STACK_TRACE` calls are
		 * made for each line in the stack trace. Each of those has the resolved
		 * [module] and [line] where the method or function is defined and [message] is
		 * the name of the method or function.
		 */
		using standard_error_handler_function_type = void (*)(gal_virtual_machine& virtual_machine, gal_error_type type, const char* module, int line, const char* message);

		/**
		 * @brief Gives the host a chance to canonicalize the imported module name,
		 * potentially taking into account the (previously resolved) name of the module
		 * that contains the import. Typically, this is used to implement relative
		 * imports.
		 */
		using resolve_module_function_type		   = const char* (*)(gal_virtual_machine& virtual_machine, const char* importer, const char* name);

		/**
		 * @brief Loads and returns the source code for the module [name].
		 */
		using load_module_function_type			   = gal_load_module_result (*)(gal_virtual_machine& virtual_machine, const char* name);

		/**
		 * @brief Called after load_module_function is called for module [name]. The original returned result
		 * is handed back to you in this callback, so that you can free memory if appropriate.
		 */
		using load_module_complete_function_type   = void (*)(gal_virtual_machine& virtual_machine, const char* name, gal_load_module_result result);

		/**
		 * @brief Returns a pointer to a outer method on [class_name] in [module] with [signature].
		 */
		using bind_outer_method_function_type	   = gal_outer_method_function_type (*)(gal_virtual_machine_state& state, const char* module, const char* class_name, bool is_static, const char* signature);

		using bind_outer_class_function_type	   = gal_outer_class_method (*)(gal_virtual_machine_state& state, const char* module, const char* class_name);

		/**
		 * @brief The callback GAL uses to display text when `print()` or the other
		 * related functions are called.
		 *
		 * If this is `nullptr`, GAL discards any printed text.
		 */
		standard_output_function_type		 standard_output_function{nullptr};

		/**
		 * @brief The callback GAL uses to report errors.
		 *
		 * When an error occurs, this will be called with the module name, line
		 * number, and an error message. If this is `nullptr`, GAL doesn't report any
		 * errors.
		 */
		standard_error_handler_function_type standard_error_handler_function{nullptr};

		/**
		 * @brief The callback GAL uses to resolve a module name.
		 *
		 * Some host applications may wish to support "relative" imports, where the
		 * meaning of an import string depends on the module that contains it. To
		 * support that without baking any policy into GAL itself, the VM gives the
		 * host a chance to resolve an import string.
		 *
		 * Before an import is loaded, it calls this, passing in the name of the
		 * module that contains the import and the import string. The host app can
		 * look at both of those and produce a new "canonical" string that uniquely
		 * identifies the module. This string is then used as the name of the module
		 * going forward. It is what is passed to [load_module_function_type], how duplicate
		 * imports of the same module are detected, and how the module is reported in
		 * stack traces.
		 *
		 * If you leave this function nullptr, then the original import string is
		 * treated as the resolved string.
		 *
		 * If an import cannot be resolved by the embedded, it should return nullptr and
		 * GAL will report that as a runtime error.
		 *
		 * GAL will take ownership of the string you return and free it for you, so
		 * it should be allocated using the same allocation function you provide
		 * above.
		 */
		resolve_module_function_type		 resolve_module_function{nullptr};

		/**
		 * @brief The callback GAL uses to load a module.
		 *
		 * Since GAL does not talk directly to the file system, it relies on the
		 * embedded to physically locate and read the source code for a module. The
		 * first time an import appears, GAL will call this and pass in the name of
		 * the module being imported. The method will return a result, which contains
		 * the source code for that module. Memory for the source is owned by the
		 * host application, and can be freed using the on_complete callback.
		 *
		 * This will only be called once for any given module name. GAL caches the
		 * result internally so subsequent imports of the same module will use the
		 * previous source and not call this.
		 *
		 * If a module with the given name could not be found by the embedded, it
		 * should return nullptr and GAL will report that as a runtime error.
		 */
		load_module_function_type			 load_module_function{nullptr};

		/**
		 * @brief The callback GAL uses to find a outer method and bind it to a class.
		 *
		 * GAL a outer method is declared in a class, this will be called with the
		 * outer method's module, class, and signature when the class body is
		 * executed. It should return a pointer to the outer function that will be
		 * bound to that method.
		 *
		 * If the outer function could not be found, this should return nullptr and
		 * GAL will report it as runtime error.
		 */
		bind_outer_method_function_type		 bind_outer_method_function{nullptr};

		/**
		 * @brief The callback GAL uses to find a outer class and get its outer methods.
		 *
		 * GAL a outer class is declared, this will be called with the class's
		 * module and name when the class body is executed. It should return the
		 * outer functions uses to allocate and (optionally) finalize the bytes
		 * stored in the outer object when an instance is created.
		 */
		bind_outer_class_function_type		 bind_outer_class_function{nullptr};

		/**
		 * @brief The number of bytes GAL will allocate before triggering the first garbage
		 * collection.
		 *
		 * defaults to 10 MB.
		 */
		gal_size_type						 initial_heap_size{1024 * 1024 * 10};

		/**
		 * @brief After a collection occurs, the threshold for the next collection is
		 * determined based on the number of bytes remaining in use. This allows GAL
		 * to shrink its memory usage automatically after reclaiming a large amount
		 * of memory.
		 *
		 * This can be used to ensure that the heap does not get too small, which can
		 * in turn lead to a large number of collections afterwards as the heap grows
		 * back to a usable size.
		 *
		 * defaults to 1 MB.
		 */
		gal_size_type						 min_heap_size{1024 * 1024};

		/**
		 * @brief GAL will resize the heap automatically as the number of bytes
		 * remaining in use after a collection changes. This number determines the
		 * amount of additional memory GAL will use after a collection, as a
		 * percentage of the current heap size.
		 *
		 * For example, say that this is 50. After a garbage collection, when there
		 * are 400 bytes of memory still in use, the next collection will be triggered
		 * after a total of 600(= 1.5 * 400) bytes are allocated (including the 400
		 * already in use.)
		 *
		 * Setting this to a smaller number wastes less memory, but triggers more
		 * frequent garbage collections.
		 */
		std::size_t							 heap_growth_percent{50};

		/**
		 * @brief User-defined data associated with the VM.
		 */
		gal_row_pointer_type				 user_data{};

		/**
		 * @brief The result of a load_module_function call.
		 * [source] is the source code for the module, or nullptr if the module is not found.
		 * [on_complete] an optional callback that will be called once GAL is done with the result.
		 */
		struct gal_load_module_result
		{
			const char*						   source{nullptr};
			load_module_complete_function_type on_complete{nullptr};
			gal_row_pointer_type			   user_data{nullptr};
		};

		struct gal_outer_class_method
		{
			constexpr static const char	   allocate_symbol_name[]	   = "<allocate>";
			constexpr static const char	   finalize_symbol_name[]	   = "<finalize>";
			constexpr static std::size_t   allocate_symbol_name_length = sizeof(allocate_symbol_name) - 1;
			constexpr static std::size_t   finalize_symbol_name_length = sizeof(finalize_symbol_name) - 1;

			/**
			 * @brief The callback invoked when the outer object is created.
			 *
			 * This must be provided. Inside the body of this, it must call
			 * [gal_set_slot_new_outer()] exactly once.
			 */
			gal_outer_method_function_type allocate{nullptr};

			/**
			 * @brief The callback invoked when the garbage collector is about to collect a
			 * outer object's memory.
			 *
			 * This may be `nullptr` if the outer class does not need to finalize.
			 */
			gal_finalize_function_type	   finalize{nullptr};
		};
	};

	class gal_virtual_machine
	{
	private:
		gal_virtual_machine_state& state;

	public:
		/**
		 * @brief Creates a new GAL virtual machine using the given [configuration].
		 *
		 * GAL will copy the configuration data, so the argument passed to this can be
		 * freed after calling this.
		 */
		explicit gal_virtual_machine(gal_configuration& configuration);

		/**
		 * @brief Disposes of all resources is use by [virtual_machine].
		 */
		~gal_virtual_machine() noexcept;

		// todo: virtual machine copy-able? move-able?

		/**
		 * @brief Immediately run the garbage collector to free unused memory.
		 */
		void						gc();

		/**
		 * @brief Runs [source], a string of GAL source code in a new fiber in [virtual_machine]
		 * in the context of resolved [module].
		 */
		gal_interpret_result		interpret(const char* module, const char* source);

		/**
		 * @brief Creates a handle that can be used to invoke a method with [signature] on
		 * using a receiver and arguments that are set up on the stack.
		 *
		 * This handle can be used repeatedly to directly invoke that method from C++
		 * code using [call].
		 *
		 * When you are done with this handle, it must be released using
		 * [release_handle].
		 */
		gal_handle*					make_callable_handle(const char* signature);

		/**
		 * @brief Calls [method], using the receiver and arguments previously set up on the stack.
		 *
		 * [method] must have been created by a call to [make_callable_handle]. The
		 * arguments to the method must be already on the stack. The receiver should be
		 * in slot 0 with the remaining arguments following it, in order. It is an
		 * error if the number of arguments provided does not match the method's
		 * signature.
		 *
		 * After this returns, you can access the return value from slot 0 on the stack.
		 */
		gal_interpret_result		call_handle(gal_handle& method);

		/**
		 * @brief Releases the reference stored in [handle]. After calling this, [handle] can
		 * no longer be used.
		 */
		void						release_handle(gal_handle& handle);

		/**
			The following functions are intended to be called from outer methods or
			finalizers. The interface GAL provides to a outer method is like a
			register machine: you are given a numbered array of slots that values can be
			read from and written to. Values always live in a slot (unless explicitly
			captured using get_slot_handle(), which ensures the garbage collector can
			find them.
			When your outer function is called, you are given one slot for the receiver
			and each argument to the method. The receiver is in slot 0 and the arguments
			are in increasingly numbered slots after that. You are free to read and
			write to those slots as you want. If you want more slots to use as scratch
			space, you can call ensure_slots() to add more.
			When your function returns, every slot except slot zero is discarded and the
			value in slot zero is used as the return value of the method. If you don't
			store a return value in that slot yourself, it will retain its previous
			value, the receiver.
			While GAL is dynamically typed, C++ is not. This means the C++ interface has to
			support the various types of primitive values a GAL variable can hold: bool,
			double, string, etc. If we supported this for every operation in the C++ API,
			there would be a combinatorial explosion of functions, like "get a
			double-valued element from a list", "insert a string key and double value
			into a map", etc.
			To avoid that, the only way to convert to and from a raw C++ value is by going
			into and out of a slot. All other functions work with values already in a
			slot. So, to add an element to a list, you put the list in one slot, and the
			element in another. Then there is a single API function insert_in_list()
			that takes the element out of that slot and puts it into the list.
			The goal of this API is to be easy to use while not compromising performance.
			The latter means it does not do type or bounds checking at runtime except
			using assertions which are generally removed from release builds. C++ is an
			unsafe language, so it's up to you to be careful to use it correctly. In
			return, you get a very fast FFI.
		 */

		/**
		 * @brief Returns the number of slots available to the current outer method.
		 */
		[[nodiscard]] gal_size_type get_slot_count() const;

		/**
		 * @brief Ensures that the outer method stack has at least [slots] available for
		 * use, growing the stack if needed.
		 *
		 * Does not shrink the stack if it has more than enough slots.
		 *
		 * It is an error to call this from a finalizer.
		 */
		void						ensure_slots(gal_slot_type slots);

		/**
		 * @brief Gets the type of the object in [slot].
		 */
		gal_object_type				get_slot_type(gal_slot_type slot);

		/**
		 * @brief Reads a boolean value from [slot].
		 *
		 * It is an error to call this if the slot does not contain a boolean value.
		 */
		bool						get_slot_boolean(gal_slot_type slot);

		/**
		 * @brief Reads a byte array from [slot].
		 *
		 * The memory for the returned string is owned by GAL. You can inspect it
		 * while in your outer method, but cannot keep a pointer to it after the
		 * function returns, since the garbage collector may reclaim it.
		 *
		 * Returns a pointer to the first byte of the array and fill [length] with the
		 * number of bytes in the array.
		 *
		 * It is an error to call this if the slot does not contain a string.
		 */
		const char*					get_slot_bytes(gal_slot_type slot, gal_size_type& length);

		/**
		 * @brief Reads a number from [slot].
		 *
		 * It is an error to call this if the slot does not contain a number.
		 */
		double						get_slot_number(gal_slot_type slot);

		/**
		 * @brief Reads a outer object from [slot] and returns a pointer to the outer data
		 * stored with it.
		 *
		 * It is an error to call this if the slot does not contain an instance of a
		 * outer class.
		 */
		gal_row_pointer_type		get_slot_outer(gal_slot_type slot);

		/**
		 * @brief Reads a string from [slot].
		 *
		 * The memory for the returned string is owned by GAL. You can inspect it
		 * while in your outer method, but cannot keep a pointer to it after the
		 * function returns, since the garbage collector may reclaim it.
		 *
		 * It is an error to call this if the slot does not contain a string.
		 */
		const char*					get_slot_string(gal_slot_type slot);

		/**
		 * @brief Creates a handle for the value stored in [slot].
		 *
		 * This will prevent the object that is referred to from being garbage collected
		 * until the handle is released by calling [release_handle()].
		 */
		gal_handle*					get_slot_handle(gal_slot_type slot);

		/**
		 * @brief Stores the boolean [value] in [slot].
		 */
		void						set_slot_boolean(gal_slot_type slot, bool value);

		/**
		 * @brief Stores the array [length] of [bytes] in [slot].
		 *
		 * The bytes are copied to a new string within GAL's heap, so you can free
		 * memory used by them after this is called.
		 */
		void						set_slot_bytes(gal_slot_type slot, const char* bytes, gal_size_type length);

		/**
		 * @brief Stores the numeric [value] in [slot].
		 */
		void						set_slot_number(gal_slot_type slot, double value);

		/**
		 * @brief Creates a new instance of the outer class stored in [class_slot] with [size]
		 * bytes of raw storage and places the resulting object in [slot].
		 *
		 * This does not invoke the outer class's constructor on the new instance. If
		 * you need that to happen, call the constructor from GAL, which will then
		 * call the allocator outer method. In there, call this to create the object
		 * and then the constructor will be invoked when the allocator returns.
		 *
		 * Returns a pointer to the outer object's data.
		 */
		void*						set_slot_outer(gal_slot_type slot, gal_slot_type class_slot, gal_size_type size);

		/**
		 * @brief Stores a new empty list in [slot].
		 */
		void						set_slot_list(gal_slot_type slot);

		/**
		 * @brief Stores a new empty map in [slot].
		 */
		void						set_slot_map(gal_slot_type slot);

		/**
		 * @brief Stores null in [slot].
		 */
		void						set_slot_null(gal_slot_type slot);

		/**
		 * @brief Stores the string [text] in [slot].
		 *
		 * The [text] is copied to a new string within GAL's heap, so you can free
		 * memory used by it after this is called. If the string may contain any null
		 * bytes in the middle, then you should use [set_slot_bytes()] instead.
		 */
		void						set_slot_string(gal_slot_type slot, const char* text, gal_size_type length);

		/**
		 * @brief Stores the value captured in [handle] in [slot].
		 *
		 * This does not release the handle for the value.
		 */
		void						set_slot_handle(gal_slot_type slot, gal_handle& handle);

		/**
		 * @brief Returns the number of elements in the list stored in [slot].
		 */
		gal_size_type				get_list_size(gal_slot_type slot);

		/**
		 * @brief Reads element [index] from the list in [list_slot] and stores it in [element_slot].
		 */
		void						get_list_element(gal_slot_type list_slot, gal_index_type index, gal_slot_type element_slot);

		/**
		 * @brief Sets the value stored at [index] in the list at [list_slot],
		 * to the value from [element_slot].
		 */
		void						set_list_element(gal_slot_type list_slot, gal_index_type index, gal_slot_type element_slot);

		/**
		 * @brief Takes the value stored at [element_slot] and inserts it into the list stored
		 * at [list_slot] at [index].
		 *
		 * As in GAL, negative indexes can be used to insert from the end. To append
		 * an element, use `-1` for the index.
		 */
		void						insert_to_list(gal_slot_type list_slot, gal_index_type index, gal_slot_type element_slot);

		/**
		 * @brief Returns the number of entries in the map stored in [slot].
		 */
		gal_size_type				get_map_size(gal_slot_type slot);

		/**
		 * @brief Returns true if the key in [key_slot] is found in the map placed in [map_slot].
		 */
		bool						get_map_contains_key(gal_slot_type map_slot, gal_slot_type key_slot);

		/**
		 * @brief Retrieves a value with the key in [key_slot] from the map in [map_slot] and
		 * stores it in [value_slot].
		 */
		void						get_map_value(gal_slot_type map_slot, gal_slot_type key_slot, gal_slot_type value_slot);

		/**
		 * @brief Takes the value stored at [value_slot] and inserts it into the map stored
		 * at [map_slot] with key [key_slot].
		 */
		void						set_map_value(gal_slot_type map_slot, gal_slot_type key_slot, gal_slot_type value_slot);

		/**
		 * @brief Removes a value from the map in [map_slot], with the key from [key_slot],
		 * and place it in [value_slot]. If not found, [value_slot] is set to null,
		 * the same behaviour as the GAL Map API.
		 */
		void						erase_map_value(gal_slot_type map_slot, gal_slot_type key_slot, gal_slot_type value_slot);

		/**
		 * @brief Looks up the top level variable with [name] in resolved [module] and stores
		 * it in [slot].
		 */
		void						get_variable(const char* module, const char* name, gal_slot_type slot);

		/**
		 * @brief Looks up the top level variable with [name] in resolved [module],
		 * returns false if not found. The module must be imported at the time,
		 * use has_module to ensure that before calling.
		 */
		bool						has_variable(const char* module, const char* name);

		/**
		 * @brief Returns true if [module] has been imported/resolved before, false if not.
		 */
		bool						has_module(const char* module);

		/**
		 * @brief Sets the current fiber to be aborted, and uses the value in [slot] as the
		 * runtime error object.
		 */
		void						abort_fiber(gal_slot_type slot);

		/**
		 * @brief Returns the user data associated with the VM.
		 */
		gal_row_pointer_type		get_user_data();

		/**
		 * @brief Sets user data associated with the VM.
		 */
		void						set_user_data(gal_row_pointer_type user_data);
	};
}// namespace gal

#endif//GAL_LANG_GAL_HPP
