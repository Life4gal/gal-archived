#pragma once

#ifndef GAL_LANG_COMMON_HPP
	#define GAL_LANG_COMMON_HPP

namespace gal
{
	// These flags are useful for debugging and hacking on GAL itself. They are not
	// intended to be used for production code. They default to off.

	/**
	 * @brief Set this to true to print out the compiled bytecode of each function.
	 */
	constexpr auto debug_dump_compiled_code = false;

	/**
	 * @brief Set this to trace each instruction as it's executed.
	 */
	constexpr auto debug_trace_instruction	= false;

	/**
	 * @brief The maximum number of module-level variables that may be defined at one time.
	 * This limitation comes from the 16 bits used for the arguments to
	 * `CODE_LOAD_MODULE_SIZE` and `CODE_STORE_MODULE_SIZE`.
	 */
	constexpr auto max_module_variables		= 1 << 16;// 65536

	/**
	 * @brief The maximum number of arguments that can be passed to a method. Note that
	 * this limitation is hardcoded in other places in the VM, in particular, the
	 * `CODE_CALL_XX` instructions assume a certain maximum number.
	 */
	constexpr auto max_parameters			= 1 << 4;// 16

	/**
	 * @brief The maximum name of a method, not including the signature. This is an
	 * arbitrary but enforced maximum just so we know how long the method name
	 * strings need to be in the parser.
	 */
	constexpr auto max_method_name			= 1 << 6;// 64

	/**
	 * @brief The maximum length of a method signature. Signatures look like:
	 *
	 *     foo        // Getter.
	 *     foo()      // No-argument method.
	 *     foo(_)     // One-argument method.
	 *     foo(_,_)   // Two-argument method.
	 *     init foo() // Constructor initializer.
	 *
	 * The maximum signature length takes into account the longest method name, the
	 * maximum number of parameters with separators between them, "init ", and "()".
	 */
	constexpr auto max_method_signature		= max_method_name + (max_parameters * 2) + 6;

	/**
	 * @brief The maximum number of fields a class can have, including inherited fields.
	 * This is explicit in the bytecode since `CODE_CLASS` and `CODE_SUBCLASS` take
	 * a single byte for the number of fields. Note that it's 255 and not 256
	 * because creating a class takes the *number* of fields, not the *highest
	 * field index*.
	 */
	constexpr auto max_fields				= (1 << 8) - 1;// 255

	// Tell the compiler that this part of the code will never be reached.
	#if defined(_MSC_VER)
		#define UNREACHABLE() __assume(0)
	#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
		#define UNREACHABLE() __builtin_unreachable()
	#else
		#define UNREACHABLE()
	#endif
}// namespace gal

#endif//GAL_LANG_COMMON_HPP
