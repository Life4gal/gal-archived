#pragma once

#ifndef GAL_LANG_FOUNDATION_BOOTSTRAP_HPP
#define GAL_LANG_FOUNDATION_BOOTSTRAP_HPP

#include <utils/format.hpp>
#include <gal/function_register.hpp>
#include <gal/foundation/dispatcher.hpp>
#include <gal/foundation/operator_register.hpp>
#include <gal/foundation/proxy_function.hpp>
#include <gal/language/common.hpp>

namespace gal::lang::foundation
{
	template<typename T>
		requires std::is_array_v<T>
	void register_array_type(const std::string_view name, engine_core& core)
	{
		using return_type = std::remove_extent_t<T>;

		core.add_type_info(name, make_type_info<T>());

		core.add_function(
				lang::container_subscript_interface_name::value,
				fun([](T& arr, std::size_t index) -> return_type&
				{
					constexpr auto extent = std::extent_v<T>;
					if (index >= extent)
					{
						throw std::range_error{
								std_format::format(
										"Array index out of range. Array size if {} but received index is{}.",
										extent,
										index)};
					}
					return arr[index];
				}));

		core.add_function(
				lang::container_subscript_interface_name::value,
				fun([](const T& arr, std::size_t index) -> const return_type&
				{
					constexpr auto extent = std::extent_v<T>;
					if (index >= extent)
					{
						throw std::range_error{
								std_format::format(
										"Array index out of range. Array size if {} but received index is{}.",
										extent,
										index)};
					}
					return arr[index];
				}));

		core.add_function(
				lang::container_size_interface_name::value,
				fun([](const auto&) { return std::extent_v<T>; }));

		// todo: maybe more interface?
	}

	/**
	 * @brief Add all comparison operators for the templated type.
	 * Used during bootstrap, also available to users.
	 *
	 * @tparam T Type to create comparison operators for.
	 */
	template<typename T>
	void register_comparison(engine_core& core)
	{
		operator_register::register_equal<T>(core);
		operator_register::register_not_equal<T>(core);
		operator_register::register_less_than<T>(core);
		operator_register::register_less_equal<T>(core);
		operator_register::register_greater_than<T>(core);
		operator_register::register_greater_equal<T>(core);
	}

	/**
	 * @brief Class consisting of only static functions.
	 * @note All default bootstrapping occurs from this class.
	 */
	class bootstrap
	{
	private:
		/**
		 * @brief Function allowing for assignment of an unknown type to any other value.
		 */
		static boxed_value unknown_assign(boxed_value lhs, const boxed_value& rhs)
		{
			if (lhs.is_undefined()) { return lhs.assign(rhs); }
			throw exception::bad_boxed_cast{"boxed_value has a set type already"};
		}

		template<typename T>
			requires std::is_arithmetic_v<T>
		static void register_arithmetic(const string_view_type name, engine_core& core)
		{
			core.add_type_info(engine_core::name_type{name}, make_type_info<T>());

			core.add_function(engine_core::name_type{name}, default_ctor<T>());
			core.add_function(engine_core::name_type{name}, fun([](const boxed_number& num) { return num.as<T>(); }));
			{
				engine_core::name_type n{lang::number_cast_interface_prefix::value};
				n.reserve(n.size() + name.size());
				n.append(name);
				core.add_function(
						std::move(n),
						fun(
								[]([[maybe_unused]] const string_type& string)
								{
									if constexpr (requires { std::from_chars(static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), std::declval<T&>()); })
									{
										T t;
										std::from_chars(string.data(), string.data() + string.size(), t);
										return t;
									}
									else { throw std::runtime_error{"Parsing given type is not supported yet"}; }
								}));
			}
			{
				engine_core::name_type n{lang::number_cast_interface_prefix::value};
				n.reserve(n.size() + name.size());
				n.append(name);
				core.add_function(
						std::move(n),
						fun([](const T t) { return t; }));
			}
		}

		/**
		 * @brief Add all arithmetic operators for PODs
		 */
		static void register_all_arithmetic_operators(engine_core& core)
		{
			core.add_function(
					lang::operator_assign_name::value,
					fun(&boxed_number::operator_assign));
			core.add_function(
					lang::operator_equal_name::value,
					fun(&boxed_number::operator_equal));
			core.add_function(
					lang::operator_not_equal_name::value,
					fun(&boxed_number::operator_not_equal));
			core.add_function(
					lang::operator_less_than_name::value,
					fun(&boxed_number::operator_less_than));
			core.add_function(
					lang::operator_less_equal_name::value,
					fun(&boxed_number::operator_less_equal));
			core.add_function(
					lang::operator_greater_than_name::value,
					fun(&boxed_number::operator_greater_than));
			core.add_function(
					lang::operator_greater_equal_name::value,
					fun(&boxed_number::operator_greater_equal));
			core.add_function(
					lang::operator_plus_name::value,
					fun(&boxed_number::operator_plus));
			core.add_function(
					lang::operator_minus_name::value,
					fun(&boxed_number::operator_minus));
			core.add_function(
					lang::operator_multiply_name::value,
					fun(&boxed_number::operator_multiply));
			core.add_function(
					lang::operator_divide_name::value,
					fun(&boxed_number::operator_divide));
			core.add_function(
					lang::operator_remainder_name::value,
					fun(&boxed_number::operator_remainder));
			core.add_function(
					lang::operator_plus_assign_name::value,
					fun(&boxed_number::operator_plus_assign));
			core.add_function(
					lang::operator_minus_assign_name::value,
					fun(&boxed_number::operator_minus_assign));
			core.add_function(
					lang::operator_multiply_assign_name::value,
					fun(&boxed_number::operator_multiply_assign));
			core.add_function(
					lang::operator_divide_assign_name::value,
					fun(&boxed_number::operator_divide_assign));
			core.add_function(
					lang::operator_remainder_assign_name::value,
					fun(&boxed_number::operator_remainder_assign));
			core.add_function(
					lang::operator_bitwise_shift_left_name::value,
					fun(&boxed_number::operator_bitwise_shift_left));
			core.add_function(
					lang::operator_bitwise_shift_right_name::value,
					fun(&boxed_number::operator_bitwise_shift_right));
			core.add_function(
					lang::operator_bitwise_and_name::value,
					fun(&boxed_number::operator_bitwise_and));
			core.add_function(
					lang::operator_bitwise_or_name::value,
					fun(&boxed_number::operator_bitwise_or));
			core.add_function(
					lang::operator_bitwise_xor_name::value,
					fun(&boxed_number::operator_bitwise_xor));
			core.add_function(
					lang::operator_bitwise_shift_left_assign_name::value,
					fun(&boxed_number::operator_bitwise_shift_left_assign));
			core.add_function(
					lang::operator_bitwise_shift_right_assign_name::value,
					fun(&boxed_number::operator_bitwise_shift_right_assign));
			core.add_function(
					lang::operator_bitwise_and_assign_name::value,
					fun(&boxed_number::operator_bitwise_and_assign));
			core.add_function(
					lang::operator_bitwise_or_assign_name::value,
					fun(&boxed_number::operator_bitwise_or_assign));
			core.add_function(
					lang::operator_bitwise_xor_assign_name::value,
					fun(&boxed_number::operator_bitwise_xor_assign));
			core.add_function(
					lang::operator_unary_not_name::value,
					fun(&boxed_number::operator_unary_not));
			core.add_function(
					lang::operator_unary_plus_name::value,
					fun(&boxed_number::operator_unary_plus));
			core.add_function(
					lang::operator_unary_minus_name::value,
					fun(&boxed_number::operator_unary_minus));
			core.add_function(
					lang::operator_unary_bitwise_complement_name::value,
					fun(&boxed_number::operator_unary_bitwise_complement));
		}

		/**
		 * @brief Create a bound function object. The first param is the function to bind
		 * the remaining parameters are the args to bind into the result.
		 */
		static boxed_value bind_function(const parameters_view_type params)
		{
			if (params.empty()) { throw exception::arity_error{1, 0}; }

			auto function = boxed_cast<proxy_function>(params.front());

			if (const auto arity = function->get_arity();
				arity != proxy_function_base::no_parameters_arity &&
				arity != static_cast<proxy_function_base::arity_size_type>(params.size() - 1)) { throw exception::arity_error{arity, static_cast<proxy_function_base::arity_size_type>(params.size() - 1)}; }

			return boxed_value{proxy_function{std::make_shared<bound_function>(std::move(function), params.sub_list(1).to<parameters_type>())}};
		}

		static bool has_guard(const proxy_function& function) noexcept
		{
			const auto f = std::dynamic_pointer_cast<const dynamic_proxy_function_base>(function);
			return f && f->has_guard();
		}

		static proxy_function get_guard(const proxy_function& function)
		{
			if (const auto f = std::dynamic_pointer_cast<const dynamic_proxy_function_base>(function);
				f && f->has_guard()) { return f->get_guard(); }
			throw std::runtime_error{"Function does not have a guard"};
		}

		/**
		 * @brief Assignment function for shared_ptr objects,
		 * does not perform a copy of the object pointed to,
		 * instead maintains the shared_ptr concept.
		 * Similar to std::const_pointer_cast(see function_clone). Used for proxy_function.
		 */
		template<typename F>
			requires(std::is_same_v<F, proxy_function> || std::is_same_v<F, proxy_function>)
		static boxed_value proxy_function_assign(boxed_value lhs, const F& rhs)
		{
			if (lhs.is_undefined() || (not lhs.type_info().is_const() && lhs.type_info().bare_equal(make_type_info<typename F::element_type>())))
			{
				lhs.assign(boxed_value{rhs});
				return lhs;
			}
			throw exception::bad_boxed_cast{"type mismatch in pointer assignment"};
		}

		template<typename FunctionType>
		static auto make_container_wrapper(const FunctionType& function)
		{
			return [function](const proxy_function_base* base) -> std::vector<boxed_value>
			{
				const auto& result = (base->*function)();

				std::vector<boxed_value> ret{};
				ret.reserve(result.size());

				std::ranges::for_each(
						result,
						[&ret](const auto& object) { ret.push_back(const_var(object)); });

				return ret;
			};
		}

		static bool has_parse_tree(const proxy_function& function) noexcept { return std::dynamic_pointer_cast<const dynamic_proxy_function_base>(function).operator bool(); }

		static decltype(auto) get_parse_tree(const proxy_function& function)
		{
			if (const auto f = std::dynamic_pointer_cast<const dynamic_proxy_function_base>(function);
				f) { return f->get_parse_tree(); }
			throw std::runtime_error{"Function does not have a parse tree"};
		}

		static void print(const std::string_view string) noexcept { std::cout << string; }

		static void println(const std::string_view string) noexcept { std::cout << string << '\n'; }

	public:
		/**
		 * @brief perform all common bootstrap functions for std::string, void and POD types.
		 */
		static void do_bootstrap(engine_core& core)
		{
			core.add_function(lang::operator_assign_name::value,
			                  fun(&unknown_assign));

			//*********************************************
			// void type
			//*********************************************
			core.add_type_info(lang::void_type_name::value,
			                   make_type_info<void>());

			//*********************************************
			// bool type & interface
			//*********************************************
			core.add_type_info(lang::boolean_type_name::value,
			                   make_type_info<bool>());

			operator_register::register_assign<bool>(core);
			operator_register::register_equal<bool>(core);
			operator_register::register_not_equal<bool>(core);
			operator_register::register_unary_not<bool>(core);

			//*********************************************
			// type info type & interface
			//*********************************************
			core.add_type_info(lang::type_info_type_name::value,
			                   make_type_info<gal_type_info>());

			core.add_function(lang::type_info_type_name::value, copy_ctor<gal_type_info>());
			operator_register::register_equal<gal_type_info>(core);

			core.add_function(lang::type_info_is_void_interface_name::value,
			                  fun(&gal_type_info::is_void));
			core.add_function(lang::type_info_is_arithmetic_interface_name::value,
			                  fun(&gal_type_info::is_arithmetic));
			core.add_function(lang::type_info_is_const_interface_name::value,
			                  fun(&gal_type_info::is_const));
			core.add_function(lang::type_info_is_reference_interface_name::value,
			                  fun(&gal_type_info::is_reference));
			core.add_function(lang::type_info_is_pointer_interface_name::value,
			                  fun(&gal_type_info::is_pointer));
			core.add_function(lang::type_info_is_undefined_interface_name::value,
			                  fun(&gal_type_info::is_undefined));
			core.add_function(lang::type_info_bare_equal_interface_name::value,
			                  fun(static_cast<bool (gal_type_info::*)(const gal_type_info& other) const noexcept>(&gal_type_info::bare_equal)));
			core.add_function(lang::type_info_name_interface_name::value,
			                  fun(&gal_type_info::name));
			core.add_function(lang::type_info_bare_name_interface_name::value,
			                  fun(&gal_type_info::bare_name));

			//*********************************************
			// object type & interface
			//*********************************************
			core.add_type_info(lang::object_type_name::value,
			                   make_type_info<boxed_value>());

			core.add_function(lang::object_type_info_interface_name::value,
			                  fun(&boxed_value::type_info));
			core.add_function(lang::object_is_undefined_interface_name::value,
			                  fun(&boxed_value::is_undefined));
			core.add_function(lang::object_is_const_interface_name::value,
			                  fun(&boxed_value::is_const));
			core.add_function(lang::object_is_null_interface_name::value,
			                  fun(&boxed_value::is_null));
			core.add_function(lang::object_is_reference_interface_name::value,
			                  fun(&boxed_value::is_reference));
			core.add_function(lang::object_is_pointer_interface_name::value,
			                  fun(&boxed_value::is_pointer));
			core.add_function(lang::object_is_xvalue_interface_name::value,
			                  fun(&boxed_value::is_xvalue));
			core.add_function(lang::object_to_lvalue_interface_name::value,
			                  fun(&boxed_value::to_lvalue));
			core.add_function(lang::object_is_type_of_interface_name::value,
			                  fun(&boxed_value::is_type_of));
			core.add_function(lang::object_get_member_interface_name::value,
			                  fun(&boxed_value::get_member_data));
			core.add_function(lang::object_clone_members_interface_name::value,
			                  fun(&boxed_value::clone_member_data));

			//*********************************************
			// number type & interface
			//*********************************************
			core.add_type_info(lang::number_type_name::value,
			                   make_type_info<boxed_number>());

			register_arithmetic<std::int8_t>(lang::number_int8_type_name::value, core);
			register_arithmetic<std::uint8_t>(lang::number_uint8_type_name::value, core);
			register_arithmetic<std::int16_t>(lang::number_int16_type_name::value, core);
			register_arithmetic<std::uint16_t>(lang::number_uint16_type_name::value, core);
			register_arithmetic<std::int32_t>(lang::number_int32_type_name::value, core);
			register_arithmetic<std::uint32_t>(lang::number_uint32_type_name::value, core);
			register_arithmetic<std::int64_t>(lang::number_int64_type_name::value, core);
			register_arithmetic<std::uint64_t>(lang::number_uint64_type_name::value, core);
			register_arithmetic<float>(lang::number_float_type_name::value, core);
			register_arithmetic<double>(lang::number_double_type_name::value, core);
			register_arithmetic<long double>(lang::number_long_double_type_name::value, core);

			register_arithmetic<char>(lang::number_char_type_name::value, core);
			register_arithmetic<unsigned char>(lang::number_unsigned_char_type_name::value, core);
			register_arithmetic<wchar_t>(lang::number_wchar_type_name::value, core);
			register_arithmetic<char8_t>(lang::number_char8_type_name::value, core);
			register_arithmetic<char16_t>(lang::number_char16_type_name::value, core);
			register_arithmetic<char32_t>(lang::number_char32_type_name::value, core);
			register_arithmetic<short>(lang::number_short_type_name::value, core);
			register_arithmetic<unsigned short>(lang::number_unsigned_short_type_name::value, core);
			register_arithmetic<int>(lang::number_int_type_name::value, core);
			register_arithmetic<unsigned int>(lang::number_unsigned_int_type_name::value, core);
			register_arithmetic<long>(lang::number_long_type_name::value, core);
			register_arithmetic<unsigned long>(lang::number_unsigned_long_type_name::value, core);
			register_arithmetic<long long>(lang::number_long_long_type_name::value, core);
			register_arithmetic<unsigned long long>(lang::number_unsigned_long_long_type_name::value, core);

			register_all_arithmetic_operators(core);

			//*********************************************
			// function & interface
			//*********************************************
			core.add_type_info(lang::function_type_name::value,
			                   make_type_info<proxy_function>());

			core.add_function(lang::function_get_arity_interface_name::value,
			                  fun(&proxy_function_base::get_arity));
			core.add_function(lang::function_equal_interface_name::value,
			                  fun(&proxy_function_base::operator==));
			core.add_function(lang::function_get_param_types_interface_name::value,
			                  fun(make_container_wrapper(&proxy_function_base::types)));
			core.add_function(lang::function_get_contained_functions_interface_name::value,
			                  fun(make_container_wrapper(&proxy_function_base::container_functions)));

			core.add_function(lang::function_has_guard_interface_name::value,
			                  fun(&has_guard));
			core.add_function(lang::function_get_guard_interface_name::value,
			                  fun(&get_guard));

			core.add_function(lang::operator_assign_name::value,
			                  fun(&proxy_function_assign<proxy_function>));

			core.add_function(lang::operator_assign_name::value,
			                  fun(&proxy_function_assign<proxy_function>));

			core.add_function(lang::function_clone_interface_name::value,
			                  fun([](const proxy_function& self) { return std::const_pointer_cast<proxy_function::element_type>(self); }));

			core.add_type_info(lang::assignable_function_type_name::value,
			                   make_type_info<proxy_function_assignable_base>());
			core.add_type_conversion(make_base_conversion<proxy_function_base, proxy_function_assignable_base>());

			core.add_function(lang::operator_assign_name::value,
			                  fun([](proxy_function_assignable_base& lhs, const proxy_function& rhs) { lhs.assign(rhs); }));

			core.add_function(lang::function_has_parse_tree_interface_name::value,
			                  fun(&has_parse_tree));
			core.add_function(lang::function_get_parse_tree_interface_name::value,
			                  fun(&get_parse_tree));

			//*********************************************
			// dynamic object & interface
			//*********************************************
			core.add_type_info(lang::dynamic_object_type_name::value,
			                   make_type_info<dynamic_object>());

			core.add_function(lang::dynamic_object_type_name::value,
			                  default_ctor<dynamic_object>());
			core.add_function(lang::dynamic_object_type_name::value,
			                  copy_ctor<dynamic_object>());
			core.add_function(lang::dynamic_object_get_type_name_interface_name::value,
			                  fun(&dynamic_object::type_name));
			core.add_function(lang::dynamic_object_get_members_interface_name::value,
			                  fun(&dynamic_object::copy_members));
			core.add_function(lang::dynamic_object_get_member_interface_name::value,
			                  fun(static_cast<boxed_value& (dynamic_object::*)(dynamic_object::class_member_data_name_type&&)>(&dynamic_object::get_member)));
			core.add_function(lang::dynamic_object_get_member_interface_name::value,
			                  fun(static_cast<const boxed_value& (dynamic_object::*)(const dynamic_object::class_member_data_name_view_type) const>(&dynamic_object::get_member)));
			core.add_function(lang::dynamic_object_has_member_interface_name::value,
			                  fun(&dynamic_object::has_member));
			core.add_function(lang::dynamic_object_set_explicit_interface_name::value,
			                  fun(&dynamic_object::set_explicit));
			core.add_function(lang::dynamic_object_is_explicit_interface_name::value,
			                  fun(&dynamic_object::is_explicit));
			core.add_function(lang::dynamic_object_method_missing_interface_name::value,
			                  fun(static_cast<boxed_value& (dynamic_object::*)(dynamic_object::class_member_data_name_type&& name)>(&dynamic_object::method_missing)));
			core.add_function(lang::dynamic_object_method_missing_interface_name::value,
			                  fun(static_cast<const boxed_value& (dynamic_object::*)(const dynamic_object::class_member_data_name_view_type name) const>(&dynamic_object::method_missing)));

			// todo: define dynamic_object's clone/assign/compare equal/compare not equal operators

			//*********************************************
			// exception
			//*********************************************
			core.add_type_info(lang::exception_type_name::value,
			                   make_type_info<std::exception>());

			core.add_type_info(lang::exception_logic_error_type_name::value,
			                   make_type_info<std::logic_error>());
			core.add_type_conversion(make_base_conversion<std::exception, std::logic_error>());

			core.add_type_info(lang::exception_out_of_range_type_name::value,
			                   make_type_info<std::out_of_range>());
			core.add_type_conversion(make_base_conversion<std::exception, std::out_of_range>());
			core.add_type_conversion(make_base_conversion<std::logic_error, std::out_of_range>());

			core.add_type_info(lang::exception_runtime_error_type_name::value,
			                   make_type_info<std::runtime_error>());
			core.add_type_conversion(make_base_conversion<std::exception, std::runtime_error>());
			// runtime_error with a std::string message constructor
			core.add_function(lang::exception_runtime_error_type_name::value,
			                  ctor<std::runtime_error(const std::string&)>());

			core.add_type_info(lang::exception_arithmetic_error::value,
			                   make_type_info<exception::arithmetic_error>());
			core.add_type_conversion(make_base_conversion<std::exception, exception::arithmetic_error>());
			core.add_type_conversion(make_base_conversion<std::runtime_error, exception::arithmetic_error>());

			register_class<exception::eval_error>(
					core,
					lang::exception_eval_error_type_name::value,
					{},
					{{lang::exception_eval_error_reason_interface_name::value,
					  fun(&exception::eval_error::reason)},
					 {lang::exception_eval_error_pretty_print_interface_name::value,
					  fun(&exception::eval_error::pretty_print)},
					 {lang::exception_eval_error_stack_trace_interface_name::value,
					  fun([](const exception::eval_error& error)
					  {
						  std::vector<boxed_value> ret;
						  ret.reserve(error.stack_traces.size());
						  std::ranges::transform(error.stack_traces,
						                         std::back_inserter(ret),
						                         &var<const lang::ast_node_tracer&>);
						  return ret;
					  })}});

			core.add_function(lang::exception_query_interface_name::value,
			                  fun([](const std::exception& e) { return std::string{e.what()}; }));

			//*********************************************
			// common operators & interface
			//*********************************************
			core.add_function(lang::operator_to_string_name::value,
			                  fun([](const std::string& string) { return string; }));
			// bool
			core.add_function(lang::operator_to_string_name::value,
			                  fun([](const bool b) { return std::string{b ? "true" : "false"}; }));
			// char
			core.add_function(lang::operator_to_string_name::value,
			                  fun([](const char c) { return std::string{1, c}; }));
			// number
			core.add_function(lang::operator_to_string_name::value,
			                  fun(&boxed_number::to_string));

			// raise exception
			core.add_function(lang::operator_raise_exception_name::value,
			                  fun([](const boxed_value& object) { throw object; }));

			// print
			core.add_function(lang::operator_print_name::value,
			                  fun(&print));
			core.add_function(lang::operator_println_name::value,
			                  fun(&println));

			// bind
			core.add_function(lang::operator_bind_name::value,
			                  fun(&bind_function));

			core.add_function(lang::operator_type_match_name::value,
			                  fun(&boxed_value::is_type_matched));

			register_class<lang::file_point>(
					core,
					lang::file_point_type_name::value,
					{default_ctor<lang::file_point>(),
					 ctor<lang::file_point(lang::file_point::size_type, lang::file_point::size_type)>()},
					{{lang::file_point_line_interface_name::value, fun(&lang::file_point::line)},
					 {lang::file_point_column_interface_name::value, fun(&lang::file_point::column)}});

			register_class<lang::ast_node>(
					core,
					lang::ast_node_type_name::value,
					{},
					{
							// {lang::ast_node_text_interface_name::value, fun(&lang::ast_node::text_)},
							{lang::ast_node_location_begin_interface_name::value, fun(&lang::ast_node::location_begin)},
							{lang::ast_node_location_end_interface_name::value, fun(&lang::ast_node::location_end)},
							{lang::ast_node_filename_interface_name::value, fun(&lang::ast_node::filename)},
							{lang::ast_node_to_string_interface_name::value, fun(&lang::ast_node::to_string)},
							{lang::ast_node_children_interface_name::value, fun(&lang::ast_node::get_boxed_children)}});
		}
	};
}

#endif // GAL_LANG_FOUNDATION_BOOTSTRAP_HPP
