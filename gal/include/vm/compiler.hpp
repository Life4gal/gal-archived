#pragma once

#ifndef GAL_LANG_COMPILER_HPP
	#define GAL_LANG_COMPILER_HPP

	#include <gal.hpp>

/**
* @brief This module defines the compiler for GAL. It takes a string of source code
* and lexes, parses, and compiles it. GAL uses a single-pass compiler. It
* does not build an actual AST during parsing and then consume that to
* generate code. Instead, the parser directly emits bytecode.
*
* This forces a few restrictions on the grammar and semantics of the language.
* Things like forward references and arbitrary lookahead are much harder. We
* get a lot in return for that, though.
*
* The implementation is much simpler since we don't need to define a bunch of
* AST data structures. More so, we don't have to deal with managing memory for
* AST objects. The compiler does almost no dynamic allocation while running.
*
* Compilation is also faster since we don't create a bunch of temporary data
* structures and destroy them after generating code.
*/

namespace gal
{
	class compiler;
	class object_function;
	class object_module;
	class object_class;

	/**
	 * @brief Compiles [source], a string of GAL source code located in [module], to an
	 * [object_function] that will execute that code when invoked. Returns `nullptr` if the
	 * source contains any syntax errors.
	 *
	 * If [is_expression] is `true`, [source] should be a single expression, and
	 * this compiles it to a function that evaluates and returns that expression.
	 * Otherwise, [source] should be a series of top level statements.
	 *
	 * If [print_errors] is `true`, any compile errors are output to stderr.
	 * Otherwise, they are silently discarded.
	 */
	object_function* compile(gal_virtual_machine_state& state, object_module& module, const char* source, bool is_expression, bool print_errors);

	/**
	 * @brief When a class is defined, its superclass is not known until runtime since
	 * class definitions are just imperative statements. Most of the bytecode for a
	 * method doesn't care, but there are two places where it matters:
	 *
	 *   - To load or store a field, we need to know the index of the field in the
	 *     instance's field array. We need to adjust this so that subclass fields
	 *     are positioned after superclass fields, and we don't know this until the
	 *     superclass is known.
	 *
	 *   - Superclass calls need to know which superclass to dispatch to.
	 *
	 * We could handle this dynamically, but that adds overhead. Instead, when a
	 * method is bound, we walk the bytecode for the function and patch it up.
	 */
	void			 set_class_method(object_class& obj_class, object_function& function);

	/**
	 * @brief Reaches all the heap-allocated objects in use by [compiler] (and all of
	 * its parents) so that they are not collected by the GC.
	 */
	void			 mark_compiler(gal_virtual_machine_state& state, compiler& compiler);

	/**
	 * @brief Returns `true` if [name] is a local variable name (starts with a lowercase
	 * letter).
	 */
	bool			 is_local_name(const char* name)
	{
		// todo: better way
		return name[0] >= 'a' && name[0] <= 'z';
	}
}// namespace gal

#endif//GAL_LANG_COMPILER_HPP
