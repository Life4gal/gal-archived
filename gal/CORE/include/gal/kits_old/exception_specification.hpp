#pragma once

#ifndef GAL_LANG_KITS_EXCEPTION_SPECIFICATION_HPP
#define GAL_LANG_KITS_EXCEPTION_SPECIFICATION_HPP

#include <gal/kits/boxed_value_cast.hpp>
#include <gal/kits/dispatch.hpp>

namespace gal::lang
{
	namespace detail
	{
		struct exception_handler_base
		{
		protected:
			template<typename T>
			static void throw_type(const kits::boxed_value& object, const dispatch_engine& engine)
			{
				try { throw engine.boxed_cast<T>(object); }
				catch (const kits::bad_boxed_cast&) {}
			}

		public:
			exception_handler_base() = default;
			exception_handler_base(const exception_handler_base&) = default;
			exception_handler_base& operator=(const exception_handler_base&) = default;
			exception_handler_base(exception_handler_base&&) = default;
			exception_handler_base& operator=(exception_handler_base&&) = default;

			virtual ~exception_handler_base() noexcept = default;

			virtual void handle(const kits::boxed_value& object, const dispatch_engine& engine) = 0;
		};

		template<typename ... T>
		struct exception_handler : exception_handler_base
		{
			void handle(const kits::boxed_value& object, const dispatch_engine& engine) override { (throw_type<T>(object, engine), ...); }
		};
	}

	/**
	 * @brief Used in the automatic unboxing of exceptions thrown during script evaluation.
	 * @note Exception specifications allow the user to tell GAL what possible exceptions are expected from the script being executed.
	 * exception_handler objects are created with the make_exception_specification() function.
	 *
	 * @code
	 *
	 * try {
	 *	eval("throw(runtime_error(\"some error here\")", make_exception_specification<int, float, double, const std::string&, const std::exception&>());
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
	 *	eval("throw(runtime_error(\"some error here\")", make_exception_specification<const std::exception&>());
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
	 *	eval<int>("not a int", make_exception_specification<const std::exception&>());
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
	using exception_handler = std::shared_ptr<detail::exception_handler_base>;

	template<typename... T>
	exception_handler make_exception_specification() { return std::make_shared<detail::exception_handler<T...>>(); }
}

#endif // GAL_LANG_KITS_EXCEPTION_SPECIFICATION_HPP
