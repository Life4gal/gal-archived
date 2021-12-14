#pragma once

#ifndef GAL_LANG_META_HPP
#define GAL_LANG_META_HPP

#include <vm/common.hpp>
#include <vm/value.hpp>

namespace gal
{
	class gal_virtual_machine_state;
	class magic_value;

	constexpr char arg_placeholder = '_';

	object_class& get_meta_class(magic_value value);

	/**
	 * @brief see value.hpp -> using primitive_function_type = bool (*)([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
	 */

	/**
	 * @brief Root object class, not superclass.
	 */
	struct meta_object : public object_class
	{
	protected:
		using object_class::object_class;

	public:
		constexpr static const char name[] = "Object";
		constexpr static const auto name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_object& instance(const gal_virtual_machine_state& state);

		/**
		 * @brief "!"
		 */
		static bool operator_not([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_not_name[] = "!";

		/**
		 * @brief "==(arg_placeholder)"
		 */
		static bool operator_eq([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_eq_name[] = "==(_)";

		/**
		 * @brief "!=(arg_placeholder)"
		 */
		static bool operator_not_eq([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_not_eq_name[] = "!=(_)";

		/**
		 * @brief "instance_of(arg_placeholder)"
		 */
		static bool operator_instance_of(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_instance_of_name[] = "instance_of(_)";

		/**
		 * @brief "to_string()"
		 */
		static bool operator_to_string([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_to_string_name[] = "to_string()";

		/**
		 * @brief "typeof()"
		 */
		static bool operator_typeof([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_typeof_name[] = "typeof()";
	};

	/**
	 * @brief A subclass of Object.
	 */
	struct meta_class : public meta_object
	{
	protected:
		using meta_object::meta_object;

	public:
		constexpr static const char name[] = "Class";
		constexpr static const auto name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_class& instance(const gal_virtual_machine_state& state);

		/**
		 * @brief "nameof()"
		 */
		static bool operator_nameof([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_nameof_name[] = "nameof()";

		/**
		 * @brief "super_type()"
		 */
		static bool operator_super_type([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_super_type_name[] = "super_type()";

		/**
		 * @brief "to_string()"
		 */
		static bool operator_to_string([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_to_string_name[] = "to_string()";

		/**
		 * @brief "attributes()"
		 */
		static bool operator_attributes([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_attributes_name[] = "attributes()";
	};

	/**
	 * @brief Object's meta-class which is a subclass of Class.
	 */
	struct meta_object_metaclass final : public meta_class
	{
	private:
		using meta_class::meta_class;

	public:
		constexpr static const char name[] = "Object metaclass";
		constexpr static const auto name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_object_metaclass& instance(const gal_virtual_machine_state& state);

		/**
		 * @brief "is_same(arg_placeholder,arg_placeholder)"
		 */
		static bool operator_is_same([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_is_same_name[] = "is_same";
	};

	struct meta_boolean : public meta_object
	{
	private:
		using meta_object::meta_object;

	public:
		constexpr static const char name[] = "Bool";
		constexpr static const auto name_length = sizeof(name) - 1;

		constexpr static const char true_name[] = "true";
		constexpr static const char false_name[] = "false";
		constexpr static const auto true_name_length = sizeof(true_name) - 1;
		constexpr static const auto false_name_length = sizeof(false_name) - 1;

		// todo: remove state
		static meta_boolean& instance(const gal_virtual_machine_state& state);

		/**
		 * @brief "!"
		 */
		static bool operator_not([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_not_name[] = "!";

		/**
		 * @brief "to_string()"
		 *
		 * todo: remove state
		 */
		static bool operator_to_string(const gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_to_string_name[] = "to_string()";
	};

	struct meta_fiber : public meta_object
	{
	private:
		using meta_object::meta_object;

	public:
		constexpr static const char name[] = "Fiber";
		constexpr static const auto name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_fiber& instance(const gal_virtual_machine_state& state);

		/**
		 * @brief "new(arg_placeholder)"
		 */
		static bool operator_new(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_new_name[] = "new(_)";

		/**
		 * @brief "abort(arg_placeholder)"
		 */
		static bool operator_abort(const gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_abort_name[] = "abort(_)";

		/**
		 * @brief "current()"
		 */
		static bool operator_current(const gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_current_name[] = "current()";

		/**
		 * @brief "suspend()"
		 */
		static bool operator_suspend(gal_virtual_machine_state& state, [[maybe_unused]] magic_value* args);
		constexpr static const char operator_suspend_name[] = "suspend()";

		/**
		 * @brief "yield()"
		 */
		static bool operator_yield_no_args(gal_virtual_machine_state& state, [[maybe_unused]] magic_value* args);
		constexpr static const char operator_yield_no_args_name[] = "yield()";

		/**
		 * @brief "yield(arg_placeholder)"
		 */
		static bool operator_yield_has_args(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_yield_has_args_name[] = "yield(_)";

		/**
		 * @brief "call()"
		 */
		static bool operator_call_no_args(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_call_no_args_name[] = "call()";

		/**
		 * @brief "call(arg_placeholder)"
		 */
		static bool operator_call_has_args(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_call_has_args_name[] = "call(_)";

		/**
		 * @brief "error()"
		 */
		static bool operator_error([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_error_name[] = "error()";

		/**
		 * @brief "done()"
		 */
		static bool operator_done([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_done_name[] = "done()";

		/**
		 * @brief "transfer()"
		 */
		static bool operator_transfer_no_args(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_transfer_no_args_name[] = "transfer()";

		/**
		 * @brief "transfer(arg_placeholder)"
		 */
		static bool operator_transfer_has_args(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_transfer_has_args_name[] = "transfer(_)";

		/**
		 * @brief "transfer_error(arg_placeholder)"
		 */
		static bool operator_transfer_error(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_transfer_error_name[] = "transfer_error(_)";

		/**
		 * @brief "try()"
		 */
		static bool operator_try_no_args(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_try_no_args_name[] = "try()";

		/**
		 * @brief "try(arg_placeholder)"
		 */
		static bool operator_try_has_args(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_try_has_args_name[] = "try(_)";
	};

	struct meta_function : public meta_object
	{
	private:
		using meta_object::meta_object;

	public:
		constexpr static const char name[] = "Function";
		constexpr static const auto name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_function& instance(const gal_virtual_machine_state& state);

		/**
		 * @brief "new(arg_placeholder)"
		 */
		static bool operator_new(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_new_name[] = "new(_)";

		/**
		 * @brief "arity()"
		 */
		static bool operator_arity([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_arity_name[] = "arity()";

		/**
		 * @brief "call()"
		 */
		static bool operator_call0(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call0_name[] = "call()";

		/**
		 * @brief "call(arg_placeholder)"
		 */
		static bool operator_call1(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call1_name[] = "call(_)";

		/**
		 * @brief see common.hpp -> max_parameters
		 */

		/**
		 * @brief "call(arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call2(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call2_name[] = "call(_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call3(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call3_name[] = "call(_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call4(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call4_name[] = "call(_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call5(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call5_name[] = "call(_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call6(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call6_name[] = "call(_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call7(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call7_name[] = "call(_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call8(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call8_name[] = "call(_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call9(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call9_name[] = "call(_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call10(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call10_name[] = "call(_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call11(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call11_name[] = "call(_,_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call12(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call12_name[] = "call(_,_,_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call13(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call13_name[] = "call(_,_,_,_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call14(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call14_name[] = "call(_,_,_,_,_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call15(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call15_name[] = "call(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static bool operator_call16(gal_virtual_machine_state& state, const magic_value* args);
		constexpr static const char operator_call16_name[] = "call(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)";
	};

	struct meta_null final : public meta_object
	{
	private:
		using meta_object::meta_object;

	public:
		constexpr static const char name[] = "Null";
		constexpr static const auto name_length = sizeof(name) - 1;

		constexpr static const char null_name[] = "null";
		constexpr static const auto null_name_length = sizeof(null_name) - 1;

		// todo: remove state
		static meta_null& instance(const gal_virtual_machine_state& state);

		/**
		 * @brief "!"
		 */
		static bool operator_not([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_not_name[] = "!";

		/**
		 * @brief "to_string()"
		 *
		 * todo: remove state
		 */
		static bool operator_to_string(const gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_to_string_name[] = "to_string()";
	};

	struct meta_number final : public meta_object
	{
	private:
		using meta_object::meta_object;

	public:
		constexpr static const char name[] = "Number";
		constexpr static const auto name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_number& instance(const gal_virtual_machine_state& state);

		/**
		 * @brief "==(arg_placeholder)"
		 */
		static bool					operator_eq([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_eq_name[] = "==(_)";

		/**
		 * @brief "!=(arg_placeholder)"
		 */
		static bool					operator_not_eq([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_not_eq_name[] = "!=(_)";

		/**
		 * @brief "from_string(arg_placeholder)"
		 */
		static bool operator_from_string(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_from_string_name[] = "from_string(_)";

		/**
		 * @brief "infinity()"
		 */
		static bool operator_infinity([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_infinity_name[] = "infinity()";

		/**
		 * @brief "nan()"
		 */
		static bool operator_nan([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_nan_name[] = "nan()";

		/**
		 * @brief "pi()"
		 */
		static bool operator_pi([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_pi_name[] = "pi()";

		/**
		 * @brief "tau()"
		 */
		static bool operator_tau([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_tau_name[] = "tau()";

		/**
		 * @brief "max()"
		 */
		static bool operator_max([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_max_name[] = "max()";

		/**
		 * @brief "min()"
		 */
		static bool operator_min([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_min_name[] = "min()";

		/**
		 * @brief "max_safe_integer()"
		 */
		static bool operator_max_safe_integer([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_max_safe_integer_name[] = "max_safe_integer()";

		/**
		 * @brief "min_safe_integer()"
		 */
		static bool operator_min_safe_integer([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_min_safe_integer_name[] = "min_safe_integer()";

		/**
		 * @brief "fraction()"
		 */
		static bool operator_fraction([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_fraction_name[] = "fraction()";

		/**
		 * @brief "truncate()"
		 */
		static bool operator_truncate([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_truncate_name[] = "truncate()";

		/**
		 * @brief "is_inf()"
		 */
		static bool operator_is_inf([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_is_inf_name[] = "is_inf()";

		/**
		 * @brief "is_nan()"
		 */
		static bool operator_is_nan([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_is_nan_name[] = "is_nan()";

		/**
		 * @brief "is_integer()"
		 */
		static bool operator_is_integer([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_is_integer_name[] = "is_integer()";

		/**
		 * @brief "sign()"
		 */
		static bool operator_sign([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_sign_name[] = "sign()";

		/**
		 * @brief "to_string()"
		 *
		 * todo: remove state
		 */
		static bool operator_to_string(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_to_string_name[] = "to_string()";

		/**
		 * @brief "+(arg_placeholder)"
		 */
		static bool operator_plus(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_plus_name[] = "+(_)";

		/**
		 * @brief "-(arg_placeholder)"
		 */
		static bool operator_minus(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_minus_name[] = "-(_)";

		/**
		 * @brief "*(arg_placeholder)"
		 */
		static bool operator_multiplies(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_multiply_name[] = "*(_)";

		/**
		 * @brief "/(arg_placeholder)"
		 */
		static bool operator_divides(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_divide_name[] = "/(_)";

		/**
		 * @brief "&(arg_placeholder)"
		 */
		static bool operator_modulus(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_mod_name[] = "%(_)";

		/**
		 * @brief "<(arg_placeholder)"
		 */
		static bool operator_less(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_less_name[] = "<(_)";

		/**
		 * @brief "<=(arg_placeholder)"
		 */
		static bool operator_less_equal(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_less_equal_name[] = "<=(_)";

		/**
		 * @brief ">(arg_placeholder)"
		 */
		static bool operator_greater(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_greater_name[] = ">(_)";

		/**
		 * @brief ">=(arg_placeholder)"
		 */
		static bool operator_greater_equal(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_greater_equal_name[] = ">=(_)";

		/**
		 * @brief "&(arg_placeholder)"
		 */
		static bool operator_bitwise_and(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_bitwise_and_name[] = "&(_)";

		/**
		 * @brief "|(arg_placeholder)"
		 */
		static bool operator_bitwise_or(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_bitwise_or_name[] = "|(_)";

		/**
		 * @brief "^(arg_placeholder)"
		 */
		static bool operator_bitwise_xor(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_bitwise_xor_name[] = "^(_)";

		/**
		 * @brief "<<(arg_placeholder)"
		 */
		static bool operator_bitwise_left_shift(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_bitwise_left_shift_name[] = "<<(_)";

		/**
		 * @brief ">>(arg_placeholder)"
		 */
		static bool operator_bitwise_right_shift(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_bitwise_right_shift_name[] = ">>(_)";

		/**
		 * @brief "~"
		 */
		static bool operator_bitwise_not(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_bitwise_not_name[] = "~";

		/**
		 * @brief "min(arg_placeholder)"
		 */
		static bool operator_cmp_min(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_cmp_min_name[] = "min(_)";

		/**
		 * @brief "max(arg_placeholder)"
		 */
		static bool operator_cmp_max(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_cmp_max_name[] = "max(_)";

		/**
		 * @brief "clamp(arg_placeholder,arg_placeholder)"
		 */
		static bool operator_clamp(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_clamp_name[] = "clamp(_,_)";

		/**
		 * @brief "abs()"
		 */
		static bool operator_abs([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_abs_name[] = "abs()";

		/**
		 * @brief "-"
		 */
		static bool operator_negate([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_negate_name[] = "-";

		/**
		 * @brief "sqrt()"
		 */
		static bool operator_sqrt([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_sqrt_name[] = "sqrt()";

		/**
		 * @brief "pow(arg_placeholder)"
		 */
		static bool operator_pow(gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_pow_name[] = "pow(_)";

		/**
		 * @brief "cos()"
		 */
		static bool operator_cos([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_cos_name[] = "cos()";

		/**
		 * @brief "sin()"
		 */
		static bool operator_sin([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_sin_name[] = "sin()";

		/**
		 * @brief "tan()"
		 */
		static bool operator_tan([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_tan_name[] = "tan()";

		/**
		 * @brief "log()"
		 */
		static bool operator_log([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_log_name[] = "log()";

		/**
		 * @brief "log2()"
		 */
		static bool operator_log2([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_log2_name[] = "log2()";

		/**
		 * @brief "exp()"
		 */
		static bool operator_exp([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_exp_name[] = "exp()";

		/**
		 * @brief "exp2()"
		 */
		static bool operator_exp2([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_exp2_name[] = "exp2()";

		/**
		 * @brief "acos()"
		 */
		static bool operator_acos([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_acos_name[] = "acos()";

		/**
		 * @brief "asin()"
		 */
		static bool operator_asin([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_asin_name[] = "asin()";

		/**
		 * @brief "atan()"
		 */
		static bool operator_atan([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_atan_name[] = "atan()";

		/**
		 * @brief "atan2(arg_placeholder)"
		 */
		static bool operator_atan2([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_atan2_name[] = "atan2(_)";

		/**
		 * @brief "cbrt()"
		 */
		static bool operator_cbrt([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_cbrt_name[] = "cbrt()";

		/**
		 * @brief "ceil()"
		 */
		static bool operator_ceil([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_ceil_name[] = "ceil()";

		/**
		 * @brief "floor()"
		 */
		static bool operator_floor([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_floor_name[] = "floor()";

		/**
		 * @brief "round()"
		 */
		static bool operator_round([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args);
		constexpr static const char operator_round_name[] = "round()";
	};
}// namespace gal

#endif//GAL_LANG_META_HPP
