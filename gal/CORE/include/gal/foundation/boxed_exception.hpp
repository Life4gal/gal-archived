#pragma once

#ifndef GAL_LANG_FOUNDATION_BOXED_EXCEPTION_HPP
#define GAL_LANG_FOUNDATION_BOXED_EXCEPTION_HPP

#include<gal/foundation/boxed_value.hpp>

namespace gal::lang
{
	namespace foundation
	{
		class boxed_exception : public std::exception
		{
		public:
			using exception::exception;
		};

		class boxed_return_exception : public boxed_exception
		{
		public:
			mutable boxed_value value;

			explicit boxed_return_exception(boxed_value v, const char* message = "throw with a boxed_value return")
				: boxed_exception{message},
				  value{std::move(v)} {}
		};
	}// namespace foundation

	namespace interrupt_type
	{
		// note: even if continue & break have the same effect, we still can't use using interrupt_xxx = foundation::boxed_exception because we need them to be different types of exceptions.

		class interrupt_return final : public foundation::boxed_return_exception
		{
		public:
			using boxed_return_exception::value;

			using boxed_return_exception::boxed_return_exception;
		};

		class interrupt_continue final : public foundation::boxed_exception
		{
		public:
			using boxed_exception::boxed_exception;
		};

		class interrupt_break final : public foundation::boxed_exception
		{
		public:
			using boxed_exception::boxed_exception;
		};
	}
}

#endif // GAL_LANG_FOUNDATION_BOXED_EXCEPTION_HPP
