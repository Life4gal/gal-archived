#pragma once

#ifndef GAL_LANG_EXTRA_OPERATOR_REGISTER_HPP
#define GAL_LANG_FOUNDATION_OPERATOR_REGISTER_HPP

#include <gal/foundation/dispatcher.hpp>
#include <gal/function_register.hpp>
#include <gal/language/name.hpp>

namespace gal::lang::extra
{
	struct operator_register
	{
		template<typename T, typename Name, typename Function>
		static void register_operator(foundation::engine_module& m, Function&& function) { m.add_function(Name::value, gal::lang::fun(std::forward<Function>(function))); }

		template<typename T, typename Name = lang::operator_assign_name>
		static void register_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs = rhs; }); }

		template<typename T, typename Name = lang::operator_assign_name>
		static void register_move_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, T&& rhs) -> T& { return lhs = std::forward<T>(rhs); }); }

		template<typename T, typename Name = lang::operator_equal_name>
		static void register_equal(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> bool { return lhs == rhs; }); }

		template<typename T, typename Name = lang::operator_not_equal_name>
		static void register_not_equal(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> bool { return lhs != rhs; }); }

		template<typename T, typename Name = lang::operator_less_than_name>
		static void register_less_than(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> bool { return lhs < rhs; }); }

		template<typename T, typename Name = lang::operator_less_equal_name>
		static void register_less_equal(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> bool { return lhs <= rhs; }); }

		template<typename T, typename Name = lang::operator_greater_than_name>
		static void register_greater_than(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> bool { return lhs > rhs; }); }

		template<typename T, typename Name = lang::operator_greater_equal_name>
		static void register_greater_equal(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> bool { return lhs >= rhs; }); }

		template<typename T, typename Name = lang::operator_plus_name>
		static void register_plus(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> T { return lhs + rhs; }); }

		template<typename T, typename Name = lang::operator_minus_name>
		static void register_minus(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> T { return lhs - rhs; }); }

		template<typename T, typename Name = lang::operator_multiply_name>
		static void register_multiply(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> T { return lhs * rhs; }); }

		template<typename T, typename Name = lang::operator_divide_name>
		static void register_divide(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> T { return lhs / rhs; }); }

		template<typename T, typename Name = lang::operator_remainder_name>
		static void register_remainder(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> T { return lhs % rhs; }); }

		template<typename T, typename Name = lang::operator_plus_assign_name>
		static void register_plus_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs += rhs; }); }

		template<typename T, typename Name = lang::operator_minus_assign_name>
		static void register_minus_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs -= rhs; }); }

		template<typename T, typename Name = lang::operator_multiply_assign_name>
		static void register_multiply_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs *= rhs; }); }

		template<typename T, typename Name = lang::operator_divide_assign_name>
		static void register_divide_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs /= rhs; }); }

		template<typename T, typename Name = lang::operator_remainder_assign_name>
		static void register_remainder_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs %= rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_shift_left_name>
		static void register_bitwise_shift_left(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> T { return lhs << rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_shift_right_name>
		static void register_bitwise_shift_right(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> T { return lhs >> rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_and_name>
		static void register_bitwise_and(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> T { return lhs & rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_or_name>
		static void register_bitwise_or(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> T { return lhs | rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_xor_name>
		static void register_bitwise_xor(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& lhs, const T& rhs) -> T { return lhs ^ rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_shift_left_assign_name>
		static void register_bitwise_shift_left_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs <<= rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_shift_right_assign_name>
		static void register_bitwise_shift_right_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs >>= rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_and_assign_name>
		static void register_bitwise_and_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs &= rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_or_assign_name>
		static void register_bitwise_or_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs |= rhs; }); }

		template<typename T, typename Name = lang::operator_bitwise_xor_assign_name>
		static void register_bitwise_xor_assign(foundation::engine_module& m) { register_operator<T, Name>(m, [](T& lhs, const T& rhs) -> T& { return lhs ^= rhs; }); }

		template<typename T, typename Name = lang::operator_unary_not_name>
		static void register_unary_not(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& self) -> decltype(auto) { return !self; }); }

		template<typename T, typename Name = lang::operator_unary_plus_name>
		static void register_unary_plus(foundation::engine_module& m)
		{
			register_operator<T, Name>(m,
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
		static void register_unary_minus(foundation::engine_module& m)
		{
			register_operator<T, Name>(m,
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
		static void register_unary_bitwise_complement(foundation::engine_module& m) { register_operator<T, Name>(m, [](const T& self) { return ~self; }); }
	};
}// namespace gal::lang::foundation

#endif//GAL_LANG_FOUNDATION_OPERATOR_REGISTER_HPP
