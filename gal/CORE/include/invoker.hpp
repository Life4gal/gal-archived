#pragma once

#ifndef GAL_LANG_INVOKER_HPP
#define GAL_LANG_INVOKER_HPP

#include <type_traits>
#include <def.hpp>

namespace gal::lang
{
	class gal_object;
	class gal_object_tuple;
	class gal_object_dictionary;

	struct gal_invoker_call
	{
		/**
		 * @brief Call a callable GAL object without any arguments
		 *
		 * callable()
		 */
		static gal_object* call(gal_object& callable);

		/**
		 * @brief Call a callable GAL object 'callable' with arguments given by the
		 * tuple 'args' and keywords arguments given by the dictionary 'kwargs'.
		 *
		 * @note 'args' must not be nullptr, use an empty tuple if no arguments are
		 * needed. If no named arguments are needed, 'pair_args' can be nullptr.
		 *
		 * callable(*args, **pair_args)
		 */
		static gal_object* call(gal_object& callable, gal_object_tuple& args, gal_object_dictionary* pair_args);

		/**
		 * @brief Call a callable GAL object 'callable', with arguments given by the
		 * tuple 'args'.  If no arguments are needed, then 'args' can be nullptr.
		 *
		 * @note Returns the result of the call on success, or nullptr on failure.
		 *
		 * callable(*args)
		 */
		static gal_object* call(gal_object& callable, gal_object_tuple* args);

		/**
		 * @brief Call a callable GAL object, callable, with a variable number of
		 * C++ arguments. The C++ arguments are described using Format.
		 *
		 * @note The Format may be empty, indicating that no arguments are provided.
		 *
		 * @note Returns the result of the call on success, or nullptr on failure.
		 *
		 * @note Format should be utils::fixed_string
		 *
		 * callable(args...)
		 */
		template<typename Format, typename... Args>
		static gal_object* call(gal_object& callable, Args&&... args);

		/**
		 * @brief Call the method named 'name' of object 'object' with a
		 * variable number of C++ arguments.  The C++ arguments are
		 * described by Format.
		 *
		 * @note The Format may be empty, indicating that no arguments are provided.
		 *
		 * @note Returns the result of the call on success, or nullptr on failure.
		 *
		 * @note Format should be utils::fixed_string
		 *
		 * object.name(args...)
		 */
		template<typename Format, typename... Args>
		static gal_object* call(gal_object& object, const char* name, Args&&... args);

		/**
		 * @brief Call a callable GAL object 'callable' with a variable number of C++
		 * arguments. The C++ arguments are provided as gal_object_* values.
		 *
		 * @note Returns the result of the call on success, or nullptr on failure.
		 *
		 * callable(args...)
		 */
		template<typename... Args>
			requires(std::is_base_of_v<gal_object, Args> && ...)
		static gal_object* call(gal_object& callable, Args&&... args);

		/**
		 * @brief Call a callable GAL object 'callable' with a variable number of C++
		 * arguments. The C++ arguments are provided as gal_object_* values.
		 *
		 * @note Returns the result of the call on success, or nullptr on failure.
		 *
		 * object.name(args...)
		 */
		template<typename... Args>
			requires(std::is_base_of_v<gal_object, Args> && ...)
		static gal_object* call(gal_object& object, gal_object& name, Args&&... args);
	};

	struct gal_invoker_math
	{
		/**
		 * @note Returns the result of the operation between lhs and rhs, or nullptr on failure.
		 */

		/**
		 * @brief Returns true if the object 'object' provides numeric protocols,
		 * and false otherwise.
		 */
		bool has_operation(gal_object& object);

		/**
		 * lhs + rhs
		 */
		gal_object* plus(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs - rhs
		 */
		gal_object* minus(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs * rhs
		 */
		gal_object* multiply(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs // rhs
		 */
		gal_object* floor_divide(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs / rhs
		 */
		gal_object* real_divide(gal_object& lhs, gal_object& rhs);

		/**
		 * divmod(lhs, rhs)
		 */
		gal_object* divide_modulus(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs % rhs
		 */
		gal_object* remainder(gal_object& lhs, gal_object& rhs);

		/**
		 * o1 ** o2 or pow(o1, o2, o3)
		 */
		gal_object* power(gal_object& object1, gal_object& object2, gal_object* object3);

		/**
		 * lhs += rhs
		 */
		gal_object* plus_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs -= rhs
		 */
		gal_object* minus_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs *= rhs
		 */
		gal_object* multiply_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs //= rhs
		 */
		gal_object* floor_divide_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs /= rhs
		 */
		gal_object* real_divide_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs %= rhs
		 */
		gal_object* remainder_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * o1 **= o2 or pow(o1, o2, o3)
		 */
		gal_object* power_assign(gal_object& object1, gal_object& object2, gal_object* object3);

		/**
		 * lhs << rhs
		 */
		gal_object* bit_left_shift(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs >> rhs
		 */
		gal_object* bit_right_shift(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs & rhs
		 */
		gal_object* bit_and(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs | rhs
		 */
		gal_object* bit_or(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs ^ rhs
		 */
		gal_object* bit_xor(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs <<= rhs
		 */
		gal_object* bit_left_shift_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs >>= rhs
		 */
		gal_object* bit_right_shift_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs &= rhs
		 */
		gal_object* bit_and_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs |= rhs
		 */
		gal_object* bit_or_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * lhs ^= rhs
		 */
		gal_object* bit_xor_assign(gal_object& lhs, gal_object& rhs);

		/**
		 * -self
		 */
		gal_object* negative(gal_object& self);

		/**
		 * +self
		 */
		gal_object* positive(gal_object& self);

		/**
		 * abs(self)
		 */
		gal_object* absolute(gal_object& self);

		/**
		 * ~self
		 */
		gal_object* invert(gal_object& self);

		/**
		 * Returns true if obj is an index integer, and 0 otherwise.
		 */
		bool has_index(gal_object& self);

		/**
		 * Returns the object 'self' converted to a GAL int,
		 * or null with an exception raised on failure.
		 */
		gal_object* index(gal_object& self);

		/**
		 * Returns the object 'self' converted to gal_size_type by going through
		 * index(self) first.
		 *
		 * If an overflow error occurs while converting the int to gal_size_type, then the
		 * second argument 'exception' is the error-type to return.
		 *
		 * If exception is nullptr, then the overflow error is cleared and the value is clipped.
		 */
		gal_size_type to_size_type(gal_object& self, gal_object* exception);

		/**
		 * int(self)
		 */
		gal_object* to_integer(gal_object& self);

		/**
		 * float(self)
		 */
		gal_object* to_floating_point(gal_object& self);

		/**
		 * Returns the integer n converted to a string with a base, with a base
		 * marker of 0b, 0o or 0x prefixed if applicable.
		 *
		 * If self is not an int object, it is converted with index(self) first.
		 */
		gal_object* to_base(gal_object& self, int base);
	};
}

#endif // GAL_LANG_INVOKER_HPP
