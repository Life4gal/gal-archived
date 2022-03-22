#pragma once

#ifndef GAL_LANG_FOUNDATION_EXCEPTION_HANDLER_HPP
#define GAL_LANG_FOUNDATION_EXCEPTION_HANDLER_HPP

#include <gal/foundation/dispatcher.hpp>

namespace gal::lang::foundation
{
	class exception_handler_base
	{
		template<typename T>
		static void cast_throw(const boxed_value& object, const dispatcher_detail::dispatcher& dispatcher)
		{
			try { throw dispatcher.template boxed_cast<T>(object); }
			catch (const exception::bad_boxed_cast&) { }
		}

	public:
		exception_handler_base() = default;
		exception_handler_base(const exception_handler_base&) = default;
		exception_handler_base& operator=(const exception_handler_base&) = default;
		exception_handler_base(exception_handler_base&&) = default;
		exception_handler_base& operator=(exception_handler_base&&) = default;
		virtual ~exception_handler_base() noexcept = default;

		virtual void handle(const boxed_value& object, const dispatcher_detail::dispatcher& dispatcher) = 0;
	};

	template<typename... T>
	class exception_handler final : public exception_handler_base
	{
	public:
		void handle(const boxed_value& object, const dispatcher_detail::dispatcher& dispatcher) override { (cast_throw<T>(object, dispatcher), ...); }
	};
}// namespace gal::lang::foundation

#endif//GAL_LANG_FOUNDATION_EXCEPTION_HANDLER_HPP
