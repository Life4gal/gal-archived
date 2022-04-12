#pragma once

#ifndef GAL_LANG_FOUNDATION_OPERATOR_REGISTER_HPP
#define GAL_LANG_FOUNDATION_OPERATOR_REGISTER_HPP

#include <gal/foundation/dispatcher.hpp>
#include <gal/function_register.hpp>
#include <gal/language/name.hpp>

namespace gal::lang::foundation
{
	struct operator_register
	{
		template<typename T, typename Name, typename Function>
		static void register_operator(engine_core& core, Function&& function) { core.add_function(Name::value, gal::lang::fun(std::forward<Function>(function))); }

		template<typename T, typename Name = lang::operator_assign_name>
		static void register_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs = rhs; }); }

		template<typename T, typename Name = lang::operator_assign_name>
		static void register_move_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, T&& rhs) -> T& { return lhs = std::forward<T>(rhs); }); }

		template<typename T, typename Name = lang::operator_equal_name>
		static void register_equal(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> bool { return lhs == rhs; }); }

		template<typename T, typename Name = lang::operator_not_equal_name>
		static void register_not_equal(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> bool { return lhs != rhs; }); }

		template<typename T, typename Name = lang::operator_less_than_name>
		static void register_less_than(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> bool { return lhs < rhs; }); }

		template<typename T, typename Name = lang::operator_less_equal_name>
		static void register_less_equal(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> bool { return lhs <= rhs; }); }

		template<typename T, typename Name = lang::operator_greater_than_name>
		static void register_greater_than(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> bool { return lhs > rhs; }); }

		template<typename T, typename Name = lang::operator_greater_equal_name>
		static void register_greater_equal(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> bool { return lhs >= rhs; }); }

		template<typename T, typename Name = lang::operator_plus_name>
		static void register_plus(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> T { return lhs + rhs; }); }

		template<typename T, typename Name = lang::operator_minus_name>
		static void register_minus(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> T { return lhs - rhs; }); }

		template<typename T, typename Name = lang::operator_multiply_name>
		static void register_multiply(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> T { return lhs * rhs; }); }

		template<typename T, typename Name = lang::operator_divide_name>
		static void register_divide(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> T { return lhs / rhs; }); }

		template<typename T, typename Name = lang::operator_remainder_name>
		static void register_remainder(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> T { return lhs % rhs; }); }

		template<typename T, typename Name = lang::operator_plus_assign_name>
		static void register_plus_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs += rhs; }); }

		template<typename T, typename Name = lang::operator_minus_assign_name>
		static void register_minus_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs -= rhs; }); }

		template<typename T, typename Name = lang::operator_multiply_assign_name>
		static void register_multiply_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs *= rhs; }); }

		template<typename T, typename Name = lang::operator_divide_assign_name>
		static void register_divide_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs /= rhs; }); }

		template<typename T, typename Name = lang::operator_remainder_assign_name>
		static void register_remainder_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs %= rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_shift_left_name>
		static void register_bitwise_shift_left(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> T { return lhs << rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_shift_right_name>
		static void register_bitwise_shift_right(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> T { return lhs >> rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_and_name>
		static void register_bitwise_and(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> T { return lhs & rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_or_name>
		static void register_bitwise_or(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> T { return lhs | rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_xor_name>
		static void register_bitwise_xor(engine_core& core) { register_operator<T, Name>(core, [](const T& lhs, const T& rhs) -> T { return lhs ^ rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_shift_left_assign_name>
		static void register_bitwise_shift_left_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs <<= rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_shift_right_assign_name>
		static void register_bitwise_shift_right_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs >>= rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_and_assign_name>
		static void register_bitwise_and_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs &= rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_or_assign_name>
		static void register_bitwise_or_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs |= rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_xor_assign_name>
		static void register_bitwise_xor_assign(engine_core& core) { register_operator<T, Name>(core, [](T& lhs, const T& rhs) -> T& { return lhs ^= rhs; }); }

		template<typename T, typename Name = lang::operator_unary_not_name>
		static void register_unary_not(engine_core& core) { register_operator<T, Name>(core, [](const T& self) -> decltype(auto) { return !self; }); }

		template<typename T, typename Name = lang::operator_unary_plus_name>
		static void register_unary_plus(engine_core& core)
		{
			register_operator<T, Name>(core,
			                           [](const T& self) -> decltype(auto)
			                           {
				                           if constexpr (std::is_signed_v<T>)
				                           {
					                           using return_type = std::make_unsigned_t<T>;
					                           if (self > 0) { return static_cast<return_type>(self); }
					                           return -static_cast<return_type>(self);
				                           }
				                           else { return +self; }
			                           });
		}

		template<typename T, typename Name = lang::operator_unary_minus_name>
		static void register_unary_minus(engine_core& core)
		{
			register_operator<T, Name>(core,
			                           [](const T& self) -> decltype(auto)
			                           {
				                           if constexpr (std::is_unsigned_v<T>)
				                           {
					                           using return_type = std::make_signed_t<T>;
					                           return -static_cast<return_type>(self);
				                           }
				                           return -self;
			                           });
		}

		template<typename T, typename Name = lang::operator_unary_bitwise_complement_name>
		static void register_unary_bitwise_complement(engine_core& core) { register_operator<T, Name>(core, [](const T& self) { return ~self; }); }
	};
}// namespace gal::lang::foundation

#endif//GAL_LANG_FOUNDATION_OPERATOR_REGISTER_HPP
