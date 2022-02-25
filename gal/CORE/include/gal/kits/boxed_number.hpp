#pragma once

#ifndef GAL_LANG_KITS_BOXED_NUMBER_HPP
#define GAL_LANG_KITS_BOXED_NUMBER_HPP

#include <gal/kits/boxed_value.hpp>
#include <gal/kits/boxed_value_cast.hpp>
#include <gal/language/algebraic.hpp>
#include <gal/utility/type_info.hpp>
#include <utils/format.hpp>

namespace gal::lang::kits
{
	class arithmetic_error final : public std::runtime_error
	{
	public:
		explicit arithmetic_error(const std::string& reason)
			: std::runtime_error{std_format::format("Arithmetic error due to '{}'", reason)} {}
	};

	/**
	 * @brief Represents any numeric type, generically.
	 * Used internally for generic operations between POD values.
	 */
	class boxed_number
	{
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
			if constexpr (not std::is_floating_point_v<T>) { if (v == 0) { throw arithmetic_error{"divide by zero"}; } }
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

		static numeric_type get_type(const boxed_value& object)
		{
			const auto& ti = object.type_info();

			if (ti == utility::make_type_info<std::int8_t>()) { return numeric_type::int8_type; }
			if (ti == utility::make_type_info<std::uint8_t>()) { return numeric_type::uint8_type; }
			if (ti == utility::make_type_info<std::int16_t>()) { return numeric_type::int16_type; }
			if (ti == utility::make_type_info<std::uint16_t>()) { return numeric_type::uint16_type; }
			if (ti == utility::make_type_info<std::int32_t>()) { return numeric_type::int32_type; }
			if (ti == utility::make_type_info<std::uint32_t>()) { return numeric_type::uint32_type; }
			if (ti == utility::make_type_info<std::int64_t>()) { return numeric_type::int64_type; }
			if (ti == utility::make_type_info<std::uint64_t>()) { return numeric_type::uint64_type; }
			if (ti == utility::make_type_info<float>()) { return numeric_type::float_type; }
			if (ti == utility::make_type_info<double>()) { return numeric_type::double_type; }
			if (ti == utility::make_type_info<long double>()) { return numeric_type::long_double_type; }

			if (ti == utility::make_type_info<char>()) { return get_integral_type<sizeof(char), std::is_signed_v<char>>(); }
			if (ti == utility::make_type_info<unsigned char>()) { return get_integral_type<sizeof(unsigned char), false>(); }
			if (ti == utility::make_type_info<wchar_t>()) { return get_integral_type<sizeof(wchar_t), std::is_signed_v<wchar_t>>(); }
			if (ti == utility::make_type_info<char8_t>()) { return get_integral_type<sizeof(char8_t), std::is_signed_v<char8_t>>(); }
			if (ti == utility::make_type_info<char16_t>()) { return get_integral_type<sizeof(char16_t), std::is_signed_v<char16_t>>(); }
			if (ti == utility::make_type_info<char32_t>()) { return get_integral_type<sizeof(char32_t), std::is_signed_v<char32_t>>(); }
			if (ti == utility::make_type_info<short>()) { return get_integral_type<sizeof(short), true>(); }
			if (ti == utility::make_type_info<unsigned short>()) { return get_integral_type<sizeof(unsigned short), false>(); }
			if (ti == utility::make_type_info<int>()) { return get_integral_type<sizeof(int), true>(); }
			if (ti == utility::make_type_info<unsigned int>()) { return get_integral_type<sizeof(unsigned int), false>(); }
			if (ti == utility::make_type_info<long>()) { return get_integral_type<sizeof(long), true>(); }
			if (ti == utility::make_type_info<unsigned long>()) { return get_integral_type<sizeof(unsigned long), false>(); }
			if (ti == utility::make_type_info<long long>()) { return get_integral_type<sizeof(long long), true>(); }
			if (ti == utility::make_type_info<unsigned long long>()) { return get_integral_type<sizeof(unsigned long long), true>(); }

			throw std::bad_any_cast{};
		}

		template<typename Callable>
		static auto visit(const boxed_value& object, Callable& function)
		{
			switch (get_type(object))
			{
					using enum numeric_type;
				case int8_type: { return function(*static_cast<const std::uint8_t*>(object.get_const_ptr())); }
				case uint8_type: { return function(*static_cast<const std::int8_t*>(object.get_const_ptr())); }
				case int16_type: { return function(*static_cast<const std::uint16_t*>(object.get_const_ptr())); }
				case uint16_type: { return function(*static_cast<const std::int16_t*>(object.get_const_ptr())); }
				case int32_type: { return function(*static_cast<const std::uint32_t*>(object.get_const_ptr())); }
				case uint32_type: { return function(*static_cast<const std::int32_t*>(object.get_const_ptr())); }
				case int64_type: { return function(*static_cast<const std::uint64_t*>(object.get_const_ptr())); }
				case uint64_type: { return function(*static_cast<const std::int64_t*>(object.get_const_ptr())); }
				case float_type: { return function(*static_cast<const float*>(object.get_const_ptr())); }
				case double_type: { return function(*static_cast<const double*>(object.get_const_ptr())); }
				case long_double_type: { return function(*static_cast<const long double*>(object.get_const_ptr())); }
			}

			throw std::bad_any_cast{};
		}

		template<typename Lhs, typename Rhs>
		struct delay_make_signed_common_type
				: std::common_type<std::make_signed_t<Lhs>, std::make_signed_t<Rhs>> { };

		template<typename Lhs, typename Rhs>
		static auto do_binary_invoke(const boxed_value& object, const algebraic_invoker::operations operation, Lhs* lhs_return, const Lhs lhs_part, const Rhs rhs_part)
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
					using enum algebraic_invoker::operations;

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
				case unary_bitwise_complement: break;
			}

			throw std::bad_any_cast{};
		}

		static auto binary_invoke(algebraic_invoker::operations operation, const boxed_value& lhs, const boxed_value& rhs)
		{
			auto lhs_visitor = [operation, &lhs, &rhs]<typename T>(const T& lhs_part)
			{
				auto rhs_visitor = [&lhs, operation, &lhs_part](const auto& rhs_part)
				{
					return do_binary_invoke(
							lhs,
							operation,
							lhs.is_return_value() ? nullptr : static_cast<T*>(lhs.get_ptr()),
							lhs_part,
							rhs_part);
				};

				return visit(
						rhs,
						rhs_visitor);
			};

			return visit(
					lhs,
					lhs_visitor
					);
		}

		static auto unary_invoke(const boxed_value& object, algebraic_invoker::operations operation)
		{
			auto unary_operator = [operation]<typename T>(const T& self)
			{
				switch (operation)// NOLINT(clang-diagnostic-switch-enum, clang-diagnostic-switch-enum)
				{
						using enum algebraic_invoker::operations;
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
					unary_operator
					);
		}

		template<typename Target, typename Source>
		static Target cast_to(const boxed_value& value) { return static_cast<Target>(*static_cast<const Source*>(value.get_const_ptr())); }

	public:
		boxed_value value;

		boxed_number()
			: value{0} {}

		explicit boxed_number(boxed_value value)
			: value{std::move(value)} { check_boxed_number(value); }

		template<typename T>
		explicit boxed_number(T t)
			: value{t} { check_boxed_number(value); }

		static void check_boxed_number(const boxed_value& value)
		{
			if (const auto& ti = value.type_info();
				ti == utility::make_type_info<bool>() || not ti.is_arithmetic()) { throw std::bad_any_cast{}; }
		}

		template<typename Source, typename Target>
		consteval static void check_type()
		{
			if constexpr (sizeof(Source) != sizeof(Target) ||
			              std::is_signed_v<Source> != std::is_signed_v<Target> ||
			              std::is_floating_point_v<Source> != std::is_floating_point_v<Target>
			) { throw std::bad_any_cast{}; }
		}

		[[nodiscard]] static bool is_floating_point(const boxed_value& value)
		{
			if (const auto& ti = value.type_info();
				ti == utility::make_type_info<float>() ||
				ti == utility::make_type_info<double>() ||
				ti == utility::make_type_info<long double>()
			) { return true; }
			return false;
		}

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

		[[nodiscard]] boxed_number as(const utility::gal_type_info& ti) const
		{
			if (ti.bare_equal(utility::make_type_info<std::int8_t>())) { return boxed_number{as<std::int8_t>()}; }
			if (ti.bare_equal(utility::make_type_info<std::uint8_t>())) { return boxed_number{as<std::uint8_t>()}; }
			if (ti.bare_equal(utility::make_type_info<std::int16_t>())) { return boxed_number{as<std::int16_t>()}; }
			if (ti.bare_equal(utility::make_type_info<std::uint16_t>())) { return boxed_number{as<std::uint16_t>()}; }
			if (ti.bare_equal(utility::make_type_info<std::int32_t>())) { return boxed_number{as<std::int32_t>()}; }
			if (ti.bare_equal(utility::make_type_info<std::uint32_t>())) { return boxed_number{as<std::uint32_t>()}; }
			if (ti.bare_equal(utility::make_type_info<std::int64_t>())) { return boxed_number{as<std::int64_t>()}; }
			if (ti.bare_equal(utility::make_type_info<std::uint64_t>())) { return boxed_number{as<std::uint64_t>()}; }
			if (ti.bare_equal(utility::make_type_info<float>())) { return boxed_number{as<float>()}; }
			if (ti.bare_equal(utility::make_type_info<double>())) { return boxed_number{as<double>()}; }
			if (ti.bare_equal(utility::make_type_info<long double>())) { return boxed_number{as<long double>()}; }

			if (ti.bare_equal(utility::make_type_info<char>())) { return boxed_number{as<char>()}; }
			if (ti.bare_equal(utility::make_type_info<unsigned char>())) { return boxed_number{as<unsigned char>()}; }
			if (ti.bare_equal(utility::make_type_info<wchar_t>())) { return boxed_number{as<wchar_t>()}; }
			if (ti.bare_equal(utility::make_type_info<char8_t>())) { return boxed_number{as<char8_t>()}; }
			if (ti.bare_equal(utility::make_type_info<char16_t>())) { return boxed_number{as<char16_t>()}; }
			if (ti.bare_equal(utility::make_type_info<char32_t>())) { return boxed_number{as<char32_t>()}; }
			if (ti.bare_equal(utility::make_type_info<short>())) { return boxed_number{as<short>()}; }
			if (ti.bare_equal(utility::make_type_info<unsigned short>())) { return boxed_number{as<unsigned short>()}; }
			if (ti.bare_equal(utility::make_type_info<int>())) { return boxed_number{as<int>()}; }
			if (ti.bare_equal(utility::make_type_info<unsigned int>())) { return boxed_number{as<unsigned int>()}; }
			if (ti.bare_equal(utility::make_type_info<long>())) { return boxed_number{as<long>()}; }
			if (ti.bare_equal(utility::make_type_info<unsigned long>())) { return boxed_number{as<unsigned long>()}; }
			if (ti.bare_equal(utility::make_type_info<long long>())) { return boxed_number{as<long long>()}; }
			if (ti.bare_equal(utility::make_type_info<unsigned long long>())) { return boxed_number{as<unsigned long long>()}; }

			throw std::bad_any_cast{};
		}

		[[nodiscard]] std::string to_string() const
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

		[[nodiscard]] static boxed_number operator_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static bool operator_equal(const boxed_number& lhs, const boxed_number& rhs) { return boxed_cast<bool>(binary_invoke(algebraic_invoker::operations::equal, lhs.value, rhs.value)); }

		[[nodiscard]] static bool operator_not_equal(const boxed_number& lhs, const boxed_number& rhs) { return boxed_cast<bool>(binary_invoke(algebraic_invoker::operations::not_equal, lhs.value, rhs.value)); }

		[[nodiscard]] static bool operator_less_than(const boxed_number& lhs, const boxed_number& rhs) { return boxed_cast<bool>(binary_invoke(algebraic_invoker::operations::less_than, lhs.value, rhs.value)); }

		[[nodiscard]] static bool operator_less_equal(const boxed_number& lhs, const boxed_number& rhs) { return boxed_cast<bool>(binary_invoke(algebraic_invoker::operations::less_equal, lhs.value, rhs.value)); }

		[[nodiscard]] static bool operator_greater_than(const boxed_number& lhs, const boxed_number& rhs) { return boxed_cast<bool>(binary_invoke(algebraic_invoker::operations::greater_than, lhs.value, rhs.value)); }

		[[nodiscard]] static bool operator_greater_equal(const boxed_number& lhs, const boxed_number& rhs) { return boxed_cast<bool>(binary_invoke(algebraic_invoker::operations::greater_equal, lhs.value, rhs.value)); }

		[[nodiscard]] static boxed_number operator_plus(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::plus, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_minus(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::minus, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_multiply(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::multiply, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_divide(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::divide, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_remainder(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::remainder, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_plus_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::plus_assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_minus_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::minus_assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_multiply_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::multiply_assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_divide_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::divide_assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_remainder_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::remainder_assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_bitwise_shift_left(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::bitwise_shift_left, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_bitwise_shift_right(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::bitwise_shift_right, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_bitwise_and(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::bitwise_and, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_bitwise_or(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::bitwise_or, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_bitwise_xor(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::bitwise_xor, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_bitwise_shift_left_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::bitwise_shift_left_assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_bitwise_shift_right_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::bitwise_shift_right_assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_bitwise_and_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::bitwise_and_assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_bitwise_or_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::bitwise_or_assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_bitwise_xor_assign(const boxed_number& lhs, const boxed_number& rhs) { return boxed_number{binary_invoke(algebraic_invoker::operations::bitwise_xor_assign, lhs.value, rhs.value)}; }

		[[nodiscard]] static boxed_number operator_unary_not(const boxed_number& self) { return boxed_number{unary_invoke(self.value, algebraic_invoker::operations::unary_not)}; }

		[[nodiscard]] static boxed_number operator_unary_plus(const boxed_number& self) { return boxed_number{unary_invoke(self.value, algebraic_invoker::operations::unary_plus)}; }

		[[nodiscard]] static boxed_number operator_unary_minus(const boxed_number& self) { return boxed_number{unary_invoke(self.value, algebraic_invoker::operations::unary_minus)}; }

		[[nodiscard]] static boxed_number operator_unary_bitwise_complement(const boxed_number& self) { return boxed_number{unary_invoke(self.value, algebraic_invoker::operations::unary_bitwise_complement)}; }
	};

	namespace detail
	{
		/**
		 * @brief cast_helper for converting from boxed_value to boxed_number
		 */
		template<>
		struct cast_helper<boxed_number>
		{
			static boxed_number cast(const boxed_value& object, const type_conversion_state*) { return boxed_number{object}; }
		};

		template<>
		struct cast_helper<const boxed_number> : cast_helper<boxed_number> {};

		template<>
		struct cast_helper<const boxed_number&> : cast_helper<boxed_number> {};
	}
}

#endif // GAL_LANG_KITS_BOXED_NUMBER_HPP
