#pragma once

#ifndef GAL_LANG_EXCEPTION_HANDLER_HPP
	#define GAL_LANG_EXCEPTION_HANDLER_HPP

	#include <gal/foundation/exception_handler.hpp>

namespace gal::lang
{
	/**
	 * @brief Used in the automatic unboxing of exceptions thrown during script evaluation.
	 * @note Exception handler allow the user to tell GAL what possible exceptions are expected from the script being executed.
	 * exception_handler objects are created with the make_exception_handler() function.
	 *
	 * @code
	 *
	 * try {
	 *	eval("throw(runtime_error(\"some error here\")", make_exception_handler<int, float, double, const std::string&, const std::exception&>());
	 * }
	 * catch(const int) {
	 * // do something
	 * }
	 * catch(float) {
	 * // do something
	 * }
	 * catch(const double) {
	 * // do something
	 * }
	 * catch(const std::string&) {
	 * // do something
	 * }
	 * catch(const std::exception&) {
	 * // do something
	 * }
	 *
	 * @endcode
	 *
	 * @note It is recommended that if catching the generic std::exception& type that you specifically catch
	 * the eval_error type, so that there is no confusion.
	 *
	 * @code
	 *
	 * try {
	 *	eval("throw(runtime_error(\"some error here\")", make_exception_handler<const std::exception&>());
	 *	}
	 *	catch(const eval_error&) {
	 *	// error in script parsing / execution
	 *	}
	 *	catch(const std::exception&) {
	 *	// error explicitly thrown from script
	 *	}
	 *
	 *	@endcode
	 *
	 * @note Similarly, if you are using the eval form that un-boxes the return value,
	 * then bad_boxed_cast should be handled as well.
	 *
	 * @code
	 *
	 * try {
	 *	eval<int>("not a int", make_exception_handler<const std::exception&>());
	 *	}
	 *	catch(const eval_error&) {
	 *	// error in script parsing / execution
	 *	}
	 *	catch(const bad_boxed_cast&) {
	 *	// error unboxing return value
	 *	}
	 *	catch(const std::exception&) {
	 *	// error explicitly thrown from script
	 *	}
	 *
	 * @endcode
	 */
	using exception_handler_type = std::shared_ptr<foundation::exception_handler_base>;

	template<typename... T>
	[[nodiscard]] exception_handler_type make_exception_handler()
	{
		return std::make_shared<foundation::exception_handler<T...>>();
	}
}// namespace gal::lang

#endif//GAL_LANG_EXCEPTION_HANDLER_HPP
