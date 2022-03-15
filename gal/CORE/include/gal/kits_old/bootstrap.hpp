#pragma once

#ifndef GAL_LANG_KITS_BOOTSTRAP_HPP
#define GAL_LANG_KITS_BOOTSTRAP_HPP

#include <iostream>
#include <utils/format.hpp>
#include <gal/kits/register_function.hpp>
#include <gal/kits/dispatch.hpp>
#include <gal/kits/operators.hpp>
#include <gal/kits/utility.hpp>
#include <gal/kits/proxy_constructor.hpp>
#include <gal/language/common.hpp>

namespace gal::lang::kits
{
	template<typename T>
		requires std::is_array_v<T>
	void register_array_type(const std::string_view name, engine_module& m)
	{
		using return_type = std::remove_extent_t<T>;

		m.add_type_info(name, utility::make_type_info<T>());

		m.add_function(
				container_subscript_interface_name::value,
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
		m.add_function(
				container_subscript_interface_name::value,
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

		m.add_function(
				container_size_interface_name::value,
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
	void register_comparison(engine_module& m)
	{
		detail::register_equal<T>(m);
		detail::register_not_equal<T>(m);
		detail::register_less_than<T>(m);
		detail::register_less_equal<T>(m);
		detail::register_greater_than<T>(m);
		detail::register_greater_equal<T>(m);
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
			throw bad_boxed_cast{"boxed_value has a set type already"};
		}

		/**
		 * @brief Add all arithmetic operators for PODs
		 */
		static void register_all_arithmetic_operators(engine_module& m)
		{
			m.add_function(
					operator_assign_name::value,
					fun(&boxed_number::operator_assign));
			m.add_function(
					operator_equal_name::value,
					fun(&boxed_number::operator_equal));
			m.add_function(
					operator_not_equal_name::value,
					fun(&boxed_number::operator_not_equal));
			m.add_function(
					operator_less_than_name::value,
					fun(&boxed_number::operator_less_than));
			m.add_function(
					operator_less_equal_name::value,
					fun(&boxed_number::operator_less_equal));
			m.add_function(
					operator_greater_than_name::value,
					fun(&boxed_number::operator_greater_than));
			m.add_function(
					operator_greater_equal_name::value,
					fun(&boxed_number::operator_greater_equal));
			m.add_function(
					operator_plus_name::value,
					fun(&boxed_number::operator_plus));
			m.add_function(
					operator_minus_name::value,
					fun(&boxed_number::operator_minus));
			m.add_function(
					operator_multiply_name::value,
					fun(&boxed_number::operator_multiply));
			m.add_function(
					operator_divide_name::value,
					fun(&boxed_number::operator_divide));
			m.add_function(
					operator_remainder_name::value,
					fun(&boxed_number::operator_remainder));
			m.add_function(
					operator_plus_assign_name::value,
					fun(&boxed_number::operator_plus_assign));
			m.add_function(
					operator_minus_assign_name::value,
					fun(&boxed_number::operator_minus_assign));
			m.add_function(
					operator_multiply_assign_name::value,
					fun(&boxed_number::operator_multiply_assign));
			m.add_function(
					operator_divide_assign_name::value,
					fun(&boxed_number::operator_divide_assign));
			m.add_function(
					operator_remainder_assign_name::value,
					fun(&boxed_number::operator_remainder_assign));
			m.add_function(
					operator_bitwise_shift_left_name::value,
					fun(&boxed_number::operator_bitwise_shift_left));
			m.add_function(
					operator_bitwise_shift_right_name::value,
					fun(&boxed_number::operator_bitwise_shift_right));
			m.add_function(
					operator_bitwise_and_name::value,
					fun(&boxed_number::operator_bitwise_and));
			m.add_function(
					operator_bitwise_or_name::value,
					fun(&boxed_number::operator_bitwise_or));
			m.add_function(
					operator_bitwise_xor_name::value,
					fun(&boxed_number::operator_bitwise_xor));
			m.add_function(
					operator_bitwise_shift_left_assign_name::value,
					fun(&boxed_number::operator_bitwise_shift_left_assign));
			m.add_function(
					operator_bitwise_shift_right_assign_name::value,
					fun(&boxed_number::operator_bitwise_shift_right_assign));
			m.add_function(
					operator_bitwise_and_assign_name::value,
					fun(&boxed_number::operator_bitwise_and_assign));
			m.add_function(
					operator_bitwise_or_assign_name::value,
					fun(&boxed_number::operator_bitwise_or_assign));
			m.add_function(
					operator_bitwise_xor_assign_name::value,
					fun(&boxed_number::operator_bitwise_xor_assign));
			m.add_function(
					operator_unary_not_name::value,
					fun(&boxed_number::operator_unary_not));
			m.add_function(
					operator_unary_plus_name::value,
					fun(&boxed_number::operator_unary_plus));
			m.add_function(
					operator_unary_minus_name::value,
					fun(&boxed_number::operator_unary_minus));
			m.add_function(
					operator_unary_bitwise_complement_name::value,
					fun(&boxed_number::operator_unary_bitwise_complement));
		}

		/**
		 * @brief Create a bound function object. The first param is the function to bind
		 * the remaining parameters are the args to bind into the result.
		 */
		static boxed_value bind_function(const function_parameters& params)
		{
			if (params.empty()) { throw arity_error{1, 0}; }

			auto function = boxed_cast<const_proxy_function>(params.front());

			if (const auto arity = function->get_arity();
				arity != proxy_function_base::no_parameters_arity &&
				arity != static_cast<proxy_function_base::arity_size_type>(params.size() - 1)) { throw arity_error{arity, static_cast<proxy_function_base::arity_size_type>(params.size() - 1)}; }

			return boxed_value{const_proxy_function{std::make_shared<bound_function>(std::move(function), bound_function::arguments_type{params.begin() + 1, params.end()})}};
		}

		static bool has_guard(const const_proxy_function& function) noexcept
		{
			const auto f = std::dynamic_pointer_cast<const base::dynamic_proxy_function_base>(function);
			return f && f->has_guard();
		}

		static const_proxy_function get_guard(const const_proxy_function& function)
		{
			if (const auto f = std::dynamic_pointer_cast<const base::dynamic_proxy_function_base>(function);
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
			requires (std::is_same_v<F, proxy_function> || std::is_same_v<F, const_proxy_function>)
		static boxed_value proxy_function_assign(boxed_value lhs, const F& rhs)
		{
			if (lhs.is_undefined() || (not lhs.type_info().is_const() && lhs.type_info().bare_equal(utility::make_type_info<typename F::element_type>())))
			{
				lhs.assign(boxed_value{rhs});
				return lhs;
			}
			throw bad_boxed_cast{"type mismatch in pointer assignment"};
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

		static bool has_parse_tree(const const_proxy_function& function) noexcept { return std::dynamic_pointer_cast<const base::dynamic_proxy_function_base>(function).operator bool(); }

		static const ast_node& get_parse_tree(const const_proxy_function& function)
		{
			if (const auto f = std::dynamic_pointer_cast<const base::dynamic_proxy_function_base>(function);
				f) { return f->get_parse_tree(); }
			throw std::runtime_error{"Function does not have a parse tree"};
		}

		static void print(const std::string_view string) noexcept { std::cout << string; }

		static void println(const std::string_view string) noexcept { std::cout << string << '\n'; }

	public:
		/**
		 * @brief perform all common bootstrap functions for std::string, void and POD types.
		 */
		static void do_bootstrap(engine_module& m)
		{
			using gal_type_info = utility::gal_type_info;

			m.add_function(operator_assign_name::value,
			               fun(&unknown_assign));

			//*********************************************
			// void type
			//*********************************************
			m.add_type_info(void_type_name::value,
			                utility::make_type_info<void>());

			//*********************************************
			// bool type & interface
			//*********************************************
			m.add_type_info(boolean_type_name::value,
			                utility::make_type_info<bool>());

			detail::register_assign<bool>(m);
			detail::register_equal<bool>(m);
			detail::register_not_equal<bool>(m);
			detail::register_unary_not<bool>(m);

			//*********************************************
			// type info type & interface
			//*********************************************
			m.add_type_info(type_info_type_name::value,
			                utility::make_type_info<gal_type_info>());

			register_copy_constructor<gal_type_info>(type_info_type_name::value, m);
			detail::register_equal<gal_type_info>(m);

			m.add_function(type_info_is_void_interface_name::value,
			               fun(&gal_type_info::is_void));
			m.add_function(type_info_is_arithmetic_interface_name::value,
			               fun(&gal_type_info::is_arithmetic));
			m.add_function(type_info_is_const_interface_name::value,
			               fun(&gal_type_info::is_const));
			m.add_function(type_info_is_reference_interface_name::value,
			               fun(&gal_type_info::is_reference));
			m.add_function(type_info_is_pointer_interface_name::value,
			               fun(&gal_type_info::is_pointer));
			m.add_function(type_info_is_undefined_interface_name::value,
			               fun(&gal_type_info::is_undefined));
			m.add_function(type_info_bare_equal_interface_name::value,
			               fun(static_cast<bool (gal_type_info::*)(const gal_type_info& other) const noexcept>(&gal_type_info::bare_equal)));
			m.add_function(type_info_name_interface_name::value,
			               fun(&gal_type_info::name));
			m.add_function(type_info_bare_name_interface_name::value,
			               fun(&gal_type_info::bare_name));

			//*********************************************
			// object type & interface
			//*********************************************
			m.add_type_info(object_type_name::value,
			                utility::make_type_info<boxed_value>());

			m.add_function(object_type_info_interface_name::value,
			               fun(&boxed_value::type_info));
			m.add_function(object_is_undefined_interface_name::value,
			               fun(&boxed_value::is_undefined));
			m.add_function(object_is_const_interface_name::value,
			               fun(&boxed_value::is_const));
			m.add_function(object_is_null_interface_name::value,
			               fun(&boxed_value::is_null));
			m.add_function(object_is_reference_interface_name::value,
			               fun(&boxed_value::is_reference));
			m.add_function(object_is_pointer_interface_name::value,
			               fun(&boxed_value::is_pointer));
			m.add_function(object_is_return_value_interface_name::value,
			               fun(&boxed_value::is_return_value));
			m.add_function(object_reset_return_value_interface_name::value,
			               fun(&boxed_value::reset_return_value));
			m.add_function(object_is_type_of_interface_name::value,
			               fun(&boxed_value::is_type_of));
			m.add_function(object_get_attribute_interface_name::value,
			               fun(&boxed_value::get_attribute));
			m.add_function(object_copy_attributes_interface_name::value,
			               fun(&boxed_value::copy_attributes));
			m.add_function(object_clone_attributes_interface_name::value,
			               fun(&boxed_value::clone_attributes));

			//*********************************************
			// number type & interface
			//*********************************************
			m.add_type_info(number_type_name::value,
			                utility::make_type_info<boxed_number>());

			register_arithmetic<std::int8_t>(number_int8_type_name::value, m);
			register_arithmetic<std::uint8_t>(number_uint8_type_name::value, m);
			register_arithmetic<std::int16_t>(number_int16_type_name::value, m);
			register_arithmetic<std::uint16_t>(number_uint16_type_name::value, m);
			register_arithmetic<std::int32_t>(number_int32_type_name::value, m);
			register_arithmetic<std::uint32_t>(number_uint32_type_name::value, m);
			register_arithmetic<std::int64_t>(number_int64_type_name::value, m);
			register_arithmetic<std::uint64_t>(number_uint64_type_name::value, m);
			register_arithmetic<float>(number_float_type_name::value, m);
			register_arithmetic<double>(number_double_type_name::value, m);
			register_arithmetic<long double>(number_long_double_type_name::value, m);

			register_arithmetic<char>(number_char_type_name::value, m);
			register_arithmetic<unsigned char>(number_unsigned_char_type_name::value, m);
			register_arithmetic<wchar_t>(number_wchar_type_name::value, m);
			register_arithmetic<char8_t>(number_char8_type_name::value, m);
			register_arithmetic<char16_t>(number_char16_type_name::value, m);
			register_arithmetic<char32_t>(number_char32_type_name::value, m);
			register_arithmetic<short>(number_short_type_name::value, m);
			register_arithmetic<unsigned short>(number_unsigned_short_type_name::value, m);
			register_arithmetic<int>(number_int_type_name::value, m);
			register_arithmetic<unsigned int>(number_unsigned_int_type_name::value, m);
			register_arithmetic<long>(number_long_type_name::value, m);
			register_arithmetic<unsigned long>(number_unsigned_long_type_name::value, m);
			register_arithmetic<long long>(number_long_long_type_name::value, m);
			register_arithmetic<unsigned long long>(number_unsigned_long_long_type_name::value, m);

			register_all_arithmetic_operators(m);

			//*********************************************
			// function & interface
			//*********************************************
			m.add_type_info(function_type_name::value,
			                utility::make_type_info<proxy_function>());

			m.add_function(function_get_arity_interface_name::value,
			               fun(&proxy_function_base::get_arity));
			m.add_function(function_equal_interface_name::value,
			               fun(&proxy_function_base::operator==));
			m.add_function(function_get_param_types_interface_name::value,
			               fun(make_container_wrapper(&proxy_function_base::types)));
			m.add_function(function_get_contained_functions_interface_name::value,
			               fun(make_container_wrapper(&proxy_function_base::get_contained_function)));

			m.add_function(function_has_guard_interface_name::value,
			               fun(&has_guard));
			m.add_function(function_get_guard_interface_name::value,
			               fun(&get_guard));

			m.add_function(operator_assign_name::value,
			               fun(&proxy_function_assign<proxy_function>));

			m.add_function(operator_assign_name::value,
			               fun(&proxy_function_assign<const_proxy_function>));

			m.add_function(function_clone_interface_name::value,
			               fun([](const const_proxy_function& self) { return std::const_pointer_cast<proxy_function::element_type>(self); }));

			m.add_type_info(assignable_function_type_name::value,
			                utility::make_type_info<base::assignable_proxy_function_base>());
			m.add_type_conversion(make_base_conversion<proxy_function_base, base::assignable_proxy_function_base>());

			m.add_function(operator_assign_name::value,
			               fun([](base::assignable_proxy_function_base& lhs, const const_proxy_function& rhs) { lhs.assign(rhs); }));

			m.add_function(function_has_parse_tree_interface_name::value,
			               fun(&has_parse_tree));
			m.add_function(function_get_parse_tree_interface_name::value,
			               fun(&get_parse_tree));

			//*********************************************
			// dynamic object & interface
			//*********************************************
			m.add_type_info(dynamic_object_type_name::value,
			                utility::make_type_info<dynamic_object>());

			m.add_function(dynamic_object_type_name::value,
			               make_constructor<dynamic_object(const dynamic_object::type_name_type&)>());
			m.add_function(dynamic_object_type_name::value,
			               make_constructor<dynamic_object()>());
			m.add_function(dynamic_object_get_type_name_interface_name::value,
			               fun(&dynamic_object::type_name));
			m.add_function(dynamic_object_get_attributes_interface_name::value,
			               fun(&dynamic_object::copy_attributes));
			m.add_function(dynamic_object_get_attribute_interface_name::value,
			               fun(static_cast<boxed_value& (dynamic_object::*)(const dynamic_object::attribute_name_type&)>(&dynamic_object::get_attribute)));
			m.add_function(dynamic_object_get_attribute_interface_name::value,
			               fun(static_cast<const boxed_value& (dynamic_object::*)(const dynamic_object::attribute_name_type&) const>(&dynamic_object::get_attribute)));
			m.add_function(dynamic_object_has_attribute_interface_name::value,
			               fun(&dynamic_object::has_attribute));
			m.add_function(dynamic_object_set_explicit_interface_name::value,
			               fun(&dynamic_object::set_explicit));
			m.add_function(dynamic_object_is_explicit_interface_name::value,
			               fun(&dynamic_object::is_explicit));
			m.add_function(dynamic_object_method_missing_interface_name::value,
			               fun(static_cast<boxed_value& (dynamic_object::*)(const dynamic_object::attribute_name_type& name)>(&dynamic_object::method_missing)));
			m.add_function(dynamic_object_method_missing_interface_name::value,
			               fun(static_cast<const boxed_value& (dynamic_object::*)(const dynamic_object::attribute_name_type& name) const>(&dynamic_object::method_missing)));

			// todo: define dynamic_object's clone/assign/compare equal/compare not equal operators

			//*********************************************
			// exception
			//*********************************************
			m.add_type_info(exception_type_name::value,
			                utility::make_type_info<std::exception>());

			m.add_type_info(exception_logic_error_type_name::value,
			                utility::make_type_info<std::logic_error>());
			m.add_type_conversion(make_base_conversion<std::exception, std::logic_error>());

			m.add_type_info(exception_out_of_range_type_name::value,
			                utility::make_type_info<std::out_of_range>());
			m.add_type_conversion(make_base_conversion<std::exception, std::out_of_range>());
			m.add_type_conversion(make_base_conversion<std::logic_error, std::out_of_range>());

			m.add_type_info(exception_runtime_error_type_name::value,
			                utility::make_type_info<std::runtime_error>());
			m.add_type_conversion(make_base_conversion<std::exception, std::runtime_error>());
			// runtime_error with a std::string message constructor
			m.add_function(exception_runtime_error_type_name::value,
			               make_constructor<std::runtime_error(const std::string&)>());

			m.add_type_info(exception_arithmetic_error::value,
			                utility::make_type_info<arithmetic_error>());
			m.add_type_conversion(make_base_conversion<std::exception, arithmetic_error>());
			m.add_type_conversion(make_base_conversion<std::runtime_error, arithmetic_error>());

			register_class<eval_error>(
					m,
					exception_eval_error_type_name::value,
					{},
					{
							{
									exception_eval_error_reason_interface_name::value,
									fun(&eval_error::reason)
							},
							{
									exception_eval_error_pretty_print_interface_name::value,
									fun(&eval_error::pretty_print)
							},
							{
									exception_eval_error_stack_trace_interface_name::value,
									fun([](const eval_error& error)
									{
										std::vector<boxed_value> ret;
										ret.reserve(error.stack_traces.size());
										std::ranges::transform(error.stack_traces,
										                       std::back_inserter(ret),
										                       &kits::var<const ast_node_trace&>);
										return ret;
									})
							}
					}
					);

			m.add_function(exception_query_interface_name::value,
			               fun([](const std::exception& e) { return std::string{e.what()}; }));

			//*********************************************
			// common operators & interface
			//*********************************************
			m.add_function(operator_to_string_name::value,
			               fun([](const std::string& string) { return string; }));
			// bool
			m.add_function(operator_to_string_name::value,
			               fun([](const bool b) { return std::string{b ? "true" : "false"}; }));
			// char
			m.add_function(operator_to_string_name::value,
			               fun([](const char c) { return std::string{1, c}; }));
			// number
			m.add_function(operator_to_string_name::value,
			               fun(&boxed_number::to_string));

			// raise exception
			m.add_function(operator_raise_exception_name::value,
			               fun([](const boxed_value& object) { throw object; }));

			// print
			m.add_function(operator_print_name::value,
			               fun(&print));
			m.add_function(operator_println_name::value,
			               fun(&println));

			// bind
			m.add_function(operator_bind_name::value,
			               fun(&bind_function));

			m.add_function(operator_type_match_name::value,
			               fun(&boxed_value::is_type_match));

			register_class<file_position>(
					m,
					file_position_type_name::value,
					{make_constructor<file_position()>(),
					 make_constructor<file_position(file_position::size_type, file_position::size_type)>()},
					{{file_position_line_interface_name::value, fun(&file_position::line)},
					 {file_position_column_interface_name::value, fun(&file_position::column)}});

			register_class<ast_node>(
					m,
					ast_node_type_name::value,
					{},
					{{ast_node_type_interface_name::value, fun(&ast_node::type)},
					 {ast_node_text_interface_name::value, fun(&ast_node::text)},
					 {ast_node_location_begin_interface_name::value, fun(&ast_node::location_begin)},
					 {ast_node_location_end_interface_name::value, fun(&ast_node::location_end)},
					 {ast_node_filename_interface_name::value, fun(&ast_node::filename)},
					 {ast_node_to_string_interface_name::value, fun(&ast_node::to_string)},
					 {ast_node_children_interface_name::value, fun([](const ast_node& node)
					 {
						 const auto& children = node.get_children();

						 std::vector<boxed_value> ret{};
						 ret.reserve(children.size());
						 std::ranges::transform(
								 children,
								 std::back_inserter(ret),
								 kits::var<const ast_node::children_type::value_type&>
								 );
						 return ret;
					 })}
					}
					);
		}
	};
}

#endif // GAL_LANG_KITS_BOOTSTRAP_HPP
