#pragma once

#ifndef GAL_LANG_FOUNDATION_EXCEPTION_HANDLER_HPP
#define GAL_LANG_FOUNDATION_EXCEPTION_HANDLER_HPP

#include <gal/foundation/dispatcher.hpp>

namespace gal::lang::foundation
{
	struct exception_handler_base
	{
		constexpr exception_handler_base() noexcept = default;
		constexpr exception_handler_base(const exception_handler_base&) = default;
		constexpr exception_handler_base& operator=(const exception_handler_base&) = default;
		constexpr exception_handler_base(exception_handler_base&&) = default;
		constexpr exception_handler_base& operator=(exception_handler_base&&) = default;
		constexpr virtual ~exception_handler_base() noexcept = default;

		// todo: handled object type?
		virtual void handle(const boxed_value& object, const dispatcher& dispatcher) = 0;
	};

	template<typename... ExceptionTypes>
	struct default_exception_handler final : exception_handler_base
	{
		using exception_types = std::tuple<ExceptionTypes...>;

	private:
		template<typename T>
		static void cast_throw(const boxed_value& object, const dispatcher& dispatcher)
		{
			try { throw dispatcher.boxed_cast<T>(object); }
			catch (const exception::bad_boxed_cast&) { }
		}

	public:
		void handle(const boxed_value& object, const dispatcher& dispatcher) override { (cast_throw<ExceptionTypes>(object, dispatcher), ...); }
	};
}

#endif // GAL_LANG_FOUNDATION_EXCEPTION_HANDLER_HPP
