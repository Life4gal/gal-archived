#pragma once

#ifndef GAL_LANG_TYPES_NUMBER_TYPE_HPP
#define GAL_LANG_TYPES_NUMBER_TYPE_HPP

#include <gal/boxed_cast.hpp>
#include <gal/boxed_value.hpp>
#include <gal/foundation/algebraic.hpp>
#include <utils/format.hpp>

namespace gal::lang
{
	namespace exception
	{
		class arithmetic_error final : public std::runtime_error
		{
		public:
			explicit arithmetic_error(const std::string_view reason)
				: std::runtime_error{std_format::format("Arithmetic error due to '{}'", reason)} {}
		};
	}// namespace exception

	namespace types
	{
		/**
		 * @brief Represents any numeric type, generically.
		 * Used internally for generic operations between POD values.
		 *
		 * todo: number_type is badly designed, redesign it!
		 */
		class number_type
		{
		public:
			static const foundation::gal_type_info& class_type() noexcept
			{
				GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,) static foundation::gal_type_info type = foundation::make_type_info<number_type>();
				return type;
			}

		private:
			enum class numeric_type
			{
				int8_type,
				uint8_type,
				int16_type,
				uint16_type,
				int32_type,
				uint32_type,
				int64_type,
				uint64_type,
				float_type,
				double_type,
				long_double_type,
			};

			template<typename T>
			constexpr static void divide_zero_protect([[maybe_unused]] T v)
			{
				#ifdef GAL_LANG_ARITHMETIC_DIVIDE_ZERO_PROTECT
				if constexpr (not std::is_floating_point_v<T>) { if (v == 0) { throw exception::arithmetic_error{"divide by zero"}; } }
				#endif
			}

			template<std::size_t TypeSize, bool Signed>
				requires(TypeSize <= sizeof(std::uint64_t))
			constexpr static numeric_type get_integral_type() noexcept
			{
				using enum numeric_type;
				if constexpr (TypeSize == sizeof(std::uint8_t))
				{
					if constexpr (Signed) { return int8_type; }
					else { return uint8_type; }
				}
				else if constexpr (TypeSize == sizeof(std::uint16_t))
				{
					if constexpr (Signed) { return int16_type; }
					else { return uint16_type; }
				}
				else if constexpr (TypeSize == sizeof(std::uint32_t))
				{
					if constexpr (Signed) { return int32_type; }
					else { return uint32_type; }
				}
				else
				{
					static_assert(TypeSize == sizeof(std::uint64_t), "Unsupported integral type");
					if constexpr (Signed) { return int64_type; }
					else { return uint64_type; }
				}
			}

			/**
			 * @throw std::bad_any_cast not support numeric type
			 */
			static numeric_type get_type(const foundation::boxed_value& object)
			{
				const auto& ti = object.type_info();

				if (ti == foundation::make_type_info<std::int8_t>()) { return numeric_type::int8_type; }
				if (ti == foundation::make_type_info<std::uint8_t>()) { return numeric_type::uint8_type; }
				if (ti == foundation::make_type_info<std::int16_t>()) { return numeric_type::int16_type; }
				if (ti == foundation::make_type_info<std::uint16_t>()) { return numeric_type::uint16_type; }
				if (ti == foundation::make_type_info<std::int32_t>()) { return numeric_type::int32_type; }
				if (ti == foundation::make_type_info<std::uint32_t>()) { return numeric_type::uint32_type; }
				if (ti == foundation::make_type_info<std::int64_t>()) { return numeric_type::int64_type; }
				if (ti == foundation::make_type_info<std::uint64_t>()) { return numeric_type::uint64_type; }
				if (ti == foundation::make_type_info<float>()) { return numeric_type::float_type; }
				if (ti == foundation::make_type_info<double>()) { return numeric_type::double_type; }
				if (ti == foundation::make_type_info<long double>()) { return numeric_type::long_double_type; }

				if (ti == foundation::make_type_info<char>()) { return get_integral_type<sizeof(char), std::is_signed_v<char>>(); }
				if (ti == foundation::make_type_info<unsigned char>()) { return get_integral_type<sizeof(unsigned char), false>(); }
				if (ti == foundation::make_type_info<wchar_t>()) { return get_integral_type<sizeof(wchar_t), std::is_signed_v<wchar_t>>(); }
				if (ti == foundation::make_type_info<char8_t>()) { return get_integral_type<sizeof(char8_t), std::is_signed_v<char8_t>>(); }
				if (ti == foundation::make_type_info<char16_t>()) { return get_integral_type<sizeof(char16_t), std::is_signed_v<char16_t>>(); }
				if (ti == foundation::make_type_info<char32_t>()) { return get_integral_type<sizeof(char32_t), std::is_signed_v<char32_t>>(); }
				if (ti == foundation::make_type_info<short>()) { return get_integral_type<sizeof(short), true>(); }
				if (ti == foundation::make_type_info<unsigned short>()) { return get_integral_type<sizeof(unsigned short), false>(); }
				if (ti == foundation::make_type_info<int>()) { return get_integral_type<sizeof(int), true>(); }
				if (ti == foundation::make_type_info<unsigned int>()) { return get_integral_type<sizeof(unsigned int), false>(); }
				if (ti == foundation::make_type_info<long>()) { return get_integral_type<sizeof(long), true>(); }
				if (ti == foundation::make_type_info<unsigned long>()) { return get_integral_type<sizeof(unsigned long), false>(); }
				if (ti == foundation::make_type_info<long long>()) { return get_integral_type<sizeof(long long), true>(); }
				if (ti == foundation::make_type_info<unsigned long long>()) { return get_integral_type<sizeof(unsigned long long), true>(); }

				throw std::bad_any_cast{};
			}

			/**
			 * @throw std::bad_any_cast not support numeric type
			 */
			template<typename Callable>
			static auto visit(const foundation::boxed_value& object, Callable& function)
			{
				switch (get_type(object))
				{
						using enum numeric_type;
					case int8_type: { return function(*static_cast<const std::uint8_t*>(object.get_const_raw())); }
					case uint8_type: { return function(*static_cast<const std::int8_t*>(object.get_const_raw())); }
					case int16_type: { return function(*static_cast<const std::uint16_t*>(object.get_const_raw())); }
					case uint16_type: { return function(*static_cast<const std::int16_t*>(object.get_const_raw())); }
					case int32_type: { return function(*static_cast<const std::uint32_t*>(object.get_const_raw())); }
					case uint32_type: { return function(*static_cast<const std::int32_t*>(object.get_const_raw())); }
					case int64_type: { return function(*static_cast<const std::uint64_t*>(object.get_const_raw())); }
					case uint64_type: { return function(*static_cast<const std::int64_t*>(object.get_const_raw())); }
					case float_type: { return function(*static_cast<const float*>(object.get_const_raw())); }
					case double_type: { return function(*static_cast<const double*>(object.get_const_raw())); }
					case long_double_type: { return function(*static_cast<const long double*>(object.get_const_raw())); }
				}

				throw std::bad_any_cast{};
			}

			template<typename Lhs, typename Rhs>
			struct delay_make_signed_common_type
					: std::common_type<std::make_signed_t<Lhs>, std::make_signed_t<Rhs>> { };

			template<typename Lhs, typename Rhs>
			static auto do_binary_invoke(const foundation::boxed_value& object, const foundation::algebraic_operations operation, Lhs* lhs_return, const Lhs lhs_part, const Rhs rhs_part)
			{
				using common_type = typename std::conditional_t<// NOLINT(misc-redundant-expression)
					(std::is_same_v<Lhs, bool> || std::is_same_v<Rhs, bool>) ||
					(std::is_floating_point_v<Lhs> || std::is_floating_point_v<Rhs>) ||
					(std::is_signed_v<Lhs> == std::is_signed_v<Rhs>),
					std::common_type<Lhs, Rhs>,
					delay_make_signed_common_type<Lhs, Rhs>>::type;

				constexpr auto floating_point_compare = []<std::floating_point T>(T lhs, T rhs) constexpr-> bool
				{
					constexpr auto epsilon = std::numeric_limits<common_type>::epsilon();
					return static_cast<common_type>(lhs) - static_cast<common_type>(rhs) < epsilon &&
					       static_cast<common_type>(rhs) - static_cast<common_type>(rhs) < epsilon;
				};

				switch (operation)
				{
						using enum foundation::algebraic_operations;

					case assign:
					{
						if (lhs_return)
						{
							*lhs_return = static_cast<Lhs>(static_cast<common_type>(rhs_part));
							return object;
						}
						break;
					}
					case equal:
					{
						if constexpr (std::is_floating_point_v<Lhs> && std::is_floating_point_v<Rhs>) { return const_var(floating_point_compare(static_cast<common_type>(lhs_part), static_cast<common_type>(rhs_part))); }
						else { return const_var(static_cast<common_type>(lhs_part) == static_cast<common_type>(rhs_part)); }
					}
					case not_equal:
					{
						if constexpr (std::is_floating_point_v<Lhs> && std::is_floating_point_v<Rhs>) { return const_var(not floating_point_compare(static_cast<common_type>(lhs_part), static_cast<common_type>(rhs_part))); }
						else { return const_var(static_cast<common_type>(lhs_part) != static_cast<common_type>(rhs_part)); }
					}

					case less_than: { return const_var(static_cast<common_type>(lhs_part) < static_cast<common_type>(rhs_part)); }
					case less_equal: { return const_var(static_cast<common_type>(lhs_part) <= static_cast<common_type>(rhs_part)); }
					case greater_than: { return const_var(static_cast<common_type>(lhs_part) > static_cast<common_type>(rhs_part)); }
					case greater_equal: { return const_var(static_cast<common_type>(lhs_part) >= static_cast<common_type>(rhs_part)); }

					case plus: { return const_var(static_cast<common_type>(lhs_part) + static_cast<common_type>(rhs_part)); }
					case minus: { return const_var(static_cast<common_type>(lhs_part) - static_cast<common_type>(rhs_part)); }
					case multiply: { return const_var(static_cast<common_type>(lhs_part) * static_cast<common_type>(rhs_part)); }
					case divide:
					{
						divide_zero_protect(rhs_part);
						return const_var(static_cast<common_type>(lhs_part) / static_cast<common_type>(rhs_part));
					}
					case remainder:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>)
						{
							divide_zero_protect(rhs_part);
							return const_var(static_cast<common_type>(lhs_part) % static_cast<common_type>(rhs_part));
						}
						else { break; }
					}

					case plus_assign:
					{
						if (lhs_return)
						{
							*lhs_return += static_cast<Lhs>(static_cast<common_type>(rhs_part));
							return object;
						}
						break;
					}
					case minus_assign:
					{
						if (lhs_return)
						{
							*lhs_return -= static_cast<Lhs>(static_cast<common_type>(rhs_part));
							return object;
						}
						break;
					}
					case multiply_assign:
					{
						if (lhs_return)
						{
							*lhs_return *= static_cast<Lhs>(static_cast<common_type>(rhs_part));
							return object;
						}
						break;
					}
					case divide_assign:
					{
						if (lhs_return)
						{
							divide_zero_protect(rhs_part);
							*lhs_return /= static_cast<Lhs>(static_cast<common_type>(rhs_part));
							return object;
						}
						break;
					}
					case remainder_assign:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>)
						{
							if (lhs_return)
							{
								divide_zero_protect(rhs_part);
								*lhs_return %= static_cast<Lhs>(static_cast<common_type>(rhs_part));
								return object;
							}
							break;
						}
						else { break; }
					}

					case bitwise_shift_left:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>) { return const_var(static_cast<common_type>(lhs_part) << static_cast<common_type>(rhs_part)); }
						else { break; }
					}
					case bitwise_shift_right:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>) { return const_var(static_cast<common_type>(lhs_part) >> static_cast<common_type>(rhs_part)); }
						else { break; }
					}
					case bitwise_and:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>) { return const_var(static_cast<common_type>(lhs_part) & static_cast<common_type>(rhs_part)); }
						else { break; }
					}
					case bitwise_or:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>) { return const_var(static_cast<common_type>(lhs_part) | static_cast<common_type>(rhs_part)); }
						else { break; }
					}
					case bitwise_xor:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>) { return const_var(static_cast<common_type>(lhs_part) ^ static_cast<common_type>(rhs_part)); }
						else { break; }
					}

					case bitwise_shift_left_assign:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>)
						{
							if (lhs_return)
							{
								*lhs_return <<= static_cast<Lhs>(static_cast<common_type>(rhs_part));
								return object;
							}
							break;
						}
						else { break; }
					}
					case bitwise_shift_right_assign:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>)
						{
							if (lhs_return)
							{
								*lhs_return >>= static_cast<Lhs>(static_cast<common_type>(rhs_part));
								return object;
							}
							break;
						}
						else { break; }
					}
					case bitwise_and_assign:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>)
						{
							if (lhs_return)
							{
								*lhs_return &= static_cast<Lhs>(static_cast<common_type>(rhs_part));
								return object;
							}
							break;
						}
						else { break; }
					}
					case bitwise_or_assign:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>)
						{
							if (lhs_return)
							{
								*lhs_return |= static_cast<Lhs>(static_cast<common_type>(rhs_part));
								return object;
							}
							break;
						}
						else { break; }
					}
					case bitwise_xor_assign:
					{
						if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>)
						{
							if (lhs_return)
							{
								*lhs_return ^= static_cast<Lhs>(static_cast<common_type>(rhs_part));
								return object;
							}
							break;
						}
						else { break; }
					}

					case unknown:
					case unary_not:
					case unary_plus:
					case unary_minus:
					case unary_bitwise_complement:
					case operations_size: break;
				}

				throw std::bad_any_cast{};
			}

			template<typename Target, typename Source>
			static Target cast_to(const foundation::boxed_value& value) { return static_cast<Target>(*static_cast<const Source*>(value.get_const_raw())); }

		public:
			/**
			 * @throw std::bad_any_cast not boolean type or arithmetic type
			 */
			static void check_boxed_number(const foundation::boxed_value& value)
			{
				if (const auto& ti = value.type_info();
					ti == foundation::make_type_info<bool>() || not ti.is_arithmetic()) { throw std::bad_any_cast{}; }
			}

			/**
			 * @throw std::bad_any_cast not the same sign or not all floating point type or the the opposite
			 */
			template<typename Source, typename Target>
			consteval static void check_type()
			{
				if constexpr (sizeof(Source) != sizeof(Target) ||
				              std::is_signed_v<Source> != std::is_signed_v<Target> ||
				              std::is_floating_point_v<Source> != std::is_floating_point_v<Target>) { throw std::bad_any_cast{}; }
			}

			[[nodiscard]] static bool is_floating_point(const foundation::boxed_value& value)
			{
				if (const auto& ti = value.type_info();
					ti == foundation::make_type_info<float>() ||
					ti == foundation::make_type_info<double>() ||
					ti == foundation::make_type_info<long double>()) { return true; }
				return false;
			}

			[[nodiscard]] static foundation::boxed_value clone(const foundation::boxed_value& object) { return number_type{object}.as(object.type_info()).value; }

			static auto binary_invoke(foundation::algebraic_operations operation, const foundation::boxed_value& lhs, const foundation::boxed_value& rhs)
			{
				auto lhs_visitor = [operation, &lhs, &rhs]<typename T>(const T& lhs_part)
				{
					auto rhs_visitor = [&lhs, operation, &lhs_part](const auto& rhs_part)
					{
						return number_type::do_binary_invoke(
								lhs,
								operation,
								lhs.is_xvalue() ? nullptr : static_cast<T*>(lhs.get_raw()),
								lhs_part,
								rhs_part);
					};

					return number_type::visit(
							rhs,
							rhs_visitor);
				};

				return visit(
						lhs,
						lhs_visitor);
			}

			static auto unary_invoke(const foundation::boxed_value& object, foundation::algebraic_operations operation)
			{
				auto unary_operator = [operation]<typename T>(const T& self)
				{
					switch (operation)// NOLINT(clang-diagnostic-switch-enum, clang-diagnostic-switch-enum)
					{
							using enum foundation::algebraic_operations;
						case unary_not:
						{
							if constexpr (std::is_integral_v<T>) { return const_var(!self); }
							else { break; }
						}
						case unary_plus: { return const_var(+self); }
						case unary_minus:
						{
							if constexpr (std::is_unsigned_v<T>) { return const_var(-static_cast<std::make_signed_t<T>>(self)); }
							else { return const_var(-self); }
						}
						case unary_bitwise_complement:
						{
							if constexpr (std::is_integral_v<T>) { return const_var(~self); }
							else { break; }
						}
						default: { break; }
					}

					throw std::bad_any_cast{};
				};

				return visit(
						object,
						unary_operator);
			}

			[[nodiscard]] static number_type operator_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static bool operator_equal(const number_type& lhs, const number_type& rhs) { return boxed_cast<bool>(binary_invoke(foundation::algebraic_operations::equal, lhs.value, rhs.value)); }

			[[nodiscard]] static bool operator_not_equal(const number_type& lhs, const number_type& rhs) { return boxed_cast<bool>(binary_invoke(foundation::algebraic_operations::not_equal, lhs.value, rhs.value)); }

			[[nodiscard]] static bool operator_less_than(const number_type& lhs, const number_type& rhs) { return boxed_cast<bool>(binary_invoke(foundation::algebraic_operations::less_than, lhs.value, rhs.value)); }

			[[nodiscard]] static bool operator_less_equal(const number_type& lhs, const number_type& rhs) { return boxed_cast<bool>(binary_invoke(foundation::algebraic_operations::less_equal, lhs.value, rhs.value)); }

			[[nodiscard]] static bool operator_greater_than(const number_type& lhs, const number_type& rhs) { return boxed_cast<bool>(binary_invoke(foundation::algebraic_operations::greater_than, lhs.value, rhs.value)); }

			[[nodiscard]] static bool operator_greater_equal(const number_type& lhs, const number_type& rhs) { return boxed_cast<bool>(binary_invoke(foundation::algebraic_operations::greater_equal, lhs.value, rhs.value)); }

			[[nodiscard]] static number_type operator_plus(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::plus, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_minus(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::minus, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_multiply(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::multiply, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_divide(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::divide, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_remainder(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::remainder, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_plus_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::plus_assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_minus_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::minus_assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_multiply_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::multiply_assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_divide_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::divide_assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_remainder_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::remainder_assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_bitwise_shift_left(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::bitwise_shift_left, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_bitwise_shift_right(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::bitwise_shift_right, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_bitwise_and(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::bitwise_and, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_bitwise_or(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::bitwise_or, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_bitwise_xor(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::bitwise_xor, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_bitwise_shift_left_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::bitwise_shift_left_assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_bitwise_shift_right_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::bitwise_shift_right_assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_bitwise_and_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::bitwise_and_assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_bitwise_or_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::bitwise_or_assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_bitwise_xor_assign(const number_type& lhs, const number_type& rhs) { return number_type{binary_invoke(foundation::algebraic_operations::bitwise_xor_assign, lhs.value, rhs.value)}; }

			[[nodiscard]] static number_type operator_unary_not(const number_type& self) { return number_type{unary_invoke(self.value, foundation::algebraic_operations::unary_not)}; }

			[[nodiscard]] static number_type operator_unary_plus(const number_type& self) { return number_type{unary_invoke(self.value, foundation::algebraic_operations::unary_plus)}; }

			[[nodiscard]] static number_type operator_unary_minus(const number_type& self) { return number_type{unary_invoke(self.value, foundation::algebraic_operations::unary_minus)}; }

			[[nodiscard]] static number_type operator_unary_bitwise_complement(const number_type& self) { return number_type{unary_invoke(self.value, foundation::algebraic_operations::unary_bitwise_complement)}; }

			foundation::boxed_value value;

			number_type()
				: value{0} {}

			explicit number_type(foundation::boxed_value&& value)
				: value{std::move(value)} { check_boxed_number(this->value); }

			explicit number_type(const foundation::boxed_value& value)
				: value{value} { check_boxed_number(this->value); }

			// template<typename T>
			// explicit boxed_number(T t)
			// 	: value{t} { check_boxed_number(value); }

			template<typename T>
				requires(std::is_same_v<T, bool> || std::is_arithmetic_v<T>)
			explicit number_type(T t)
				: value{t}
			{
				// check_boxed_number(value);
			}

			/**
			 * @throw std::bad_any_cast not supported numeric type
			 */
			template<typename Target>
			[[nodiscard]] Target as() const
			{
				switch (get_type(value))
				{
						using enum numeric_type;
					case int8_type: { return cast_to<Target, std::int8_t>(value); }
					case uint8_type: { return cast_to<Target, std::uint8_t>(value); }
					case int16_type: { return cast_to<Target, std::int16_t>(value); }
					case uint16_type: { return cast_to<Target, std::uint16_t>(value); }
					case int32_type: { return cast_to<Target, std::int32_t>(value); }
					case uint32_type: { return cast_to<Target, std::uint32_t>(value); }
					case int64_type: { return cast_to<Target, std::int64_t>(value); }
					case uint64_type: { return cast_to<Target, std::uint64_t>(value); }
					case float_type: { return cast_to<Target, float>(value); }
					case double_type: { return cast_to<Target, double>(value); }
					case long_double_type: { return cast_to<Target, long double>(value); }
				}

				throw std::bad_any_cast{};
			}

			/**
			 * @throw std::bad_any_cast not supported numeric type
			 */
			template<typename Target>
			[[nodiscard]] Target as_checked() const
			{
				switch (get_type(value))
				{
						using enum numeric_type;
					case int8_type:
					{
						check_type<std::int8_t, Target>();
						return cast_to<Target, std::int8_t>(value);
					}
					case uint8_type:
					{
						check_type<std::uint8_t, Target>();
						return cast_to<Target, std::uint8_t>(value);
					}
					case int16_type:
					{
						check_type<std::int16_t, Target>();
						return cast_to<Target, std::int16_t>(value);
					}
					case uint16_type:
					{
						check_type<std::uint16_t, Target>();
						return cast_to<Target, std::uint16_t>(value);
					}
					case int32_type:
					{
						check_type<std::int32_t, Target>();
						return cast_to<Target, std::int32_t>(value);
					}
					case uint32_type:
					{
						check_type<std::uint32_t, Target>();
						return cast_to<Target, std::uint32_t>(value);
					}
					case int64_type:
					{
						check_type<std::int64_t, Target>();
						return cast_to<Target, std::int64_t>(value);
					}
					case uint64_type:
					{
						check_type<std::uint64_t, Target>();
						return cast_to<Target, std::uint64_t>(value);
					}
					case float_type:
					{
						check_type<float, Target>();
						return cast_to<Target, float>(value);
					}
					case double_type:
					{
						check_type<double, Target>();
						return cast_to<Target, double>(value);
					}
					case long_double_type:
					{
						check_type<long double, Target>();
						return cast_to<Target, long double>(value);
					}
				}

				throw std::bad_any_cast{};
			}

			/**
			 * @throw std::bad_any_cast not supported numeric type
			 */
			[[nodiscard]] number_type as(const foundation::gal_type_info& ti) const
			{
				if (ti.bare_equal(foundation::make_type_info<std::int8_t>())) { return number_type{as<std::int8_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<std::uint8_t>())) { return number_type{as<std::uint8_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<std::int16_t>())) { return number_type{as<std::int16_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<std::uint16_t>())) { return number_type{as<std::uint16_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<std::int32_t>())) { return number_type{as<std::int32_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<std::uint32_t>())) { return number_type{as<std::uint32_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<std::int64_t>())) { return number_type{as<std::int64_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<std::uint64_t>())) { return number_type{as<std::uint64_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<float>())) { return number_type{as<float>()}; }
				if (ti.bare_equal(foundation::make_type_info<double>())) { return number_type{as<double>()}; }
				if (ti.bare_equal(foundation::make_type_info<long double>())) { return number_type{as<long double>()}; }

				if (ti.bare_equal(foundation::make_type_info<char>())) { return number_type{as<char>()}; }
				if (ti.bare_equal(foundation::make_type_info<unsigned char>())) { return number_type{as<unsigned char>()}; }
				if (ti.bare_equal(foundation::make_type_info<wchar_t>())) { return number_type{as<wchar_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<char8_t>())) { return number_type{as<char8_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<char16_t>())) { return number_type{as<char16_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<char32_t>())) { return number_type{as<char32_t>()}; }
				if (ti.bare_equal(foundation::make_type_info<short>())) { return number_type{as<short>()}; }
				if (ti.bare_equal(foundation::make_type_info<unsigned short>())) { return number_type{as<unsigned short>()}; }
				if (ti.bare_equal(foundation::make_type_info<int>())) { return number_type{as<int>()}; }
				if (ti.bare_equal(foundation::make_type_info<unsigned int>())) { return number_type{as<unsigned int>()}; }
				if (ti.bare_equal(foundation::make_type_info<long>())) { return number_type{as<long>()}; }
				if (ti.bare_equal(foundation::make_type_info<unsigned long>())) { return number_type{as<unsigned long>()}; }
				if (ti.bare_equal(foundation::make_type_info<long long>())) { return number_type{as<long long>()}; }
				if (ti.bare_equal(foundation::make_type_info<unsigned long long>())) { return number_type{as<unsigned long long>()}; }

				throw std::bad_any_cast{};
			}

			/**
			 * @throw std::bad_any_cast not supported numeric type
			 */
			[[nodiscard]] foundation::string_type to_string() const
			{
				switch (get_type(value))
				{
						using enum numeric_type;
					case int8_type: { return std_format::format("{}", as<std::int8_t>()); }
					case uint8_type: { return std_format::format("{}", as<std::uint8_t>()); }
					case int16_type: { return std_format::format("{}", as<std::int16_t>()); }
					case uint16_type: { return std_format::format("{}", as<std::uint16_t>()); }
					case int32_type: { return std_format::format("{}", as<std::int32_t>()); }
					case uint32_type: { return std_format::format("{}", as<std::uint32_t>()); }
					case int64_type: { return std_format::format("{}", as<std::int64_t>()); }
					case uint64_type: { return std_format::format("{}", as<std::uint64_t>()); }
					case float_type: { return std_format::format("{}", as<float>()); }
					case double_type: { return std_format::format("{}", as<double>()); }
					case long_double_type: { return std_format::format("{}", as<long double>()); }
				}

				throw std::bad_any_cast{};
			}
		};
	}// namespace types

	namespace foundation::boxed_cast_detail
	{
		/**
		 * @brief cast_helper for converting from boxed_value to boxed_number
		 */
		template<>
		struct cast_helper<types::number_type>
		{
			static types::number_type cast(boxed_value&& object, const lang::foundation::convertor_manager_state*) { return types::number_type{std::move(object)}; }

			static types::number_type cast(const boxed_value& object, const lang::foundation::convertor_manager_state*) { return types::number_type{object}; }
		};

		template<>
		struct cast_helper<const types::number_type> : cast_helper<types::number_type> { };

		template<>
		struct cast_helper<const types::number_type&> : cast_helper<types::number_type> { };
	}// namespace foundation::boxed_cast_detail
}    // namespace gal::lang

#endif// GAL_LANG_TYPES_NUMBER_TYPE_HPP
