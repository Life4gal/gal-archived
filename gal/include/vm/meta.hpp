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

	object_class&  get_meta_class(magic_value value);

	/**
	 * @brief Root object class, not superclass.
	 */
	struct meta_object : public object_class
	{
	protected:
		using object_class::object_class;

	public:
		constexpr static const char name[]		= "Object";
		constexpr static const auto name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_object&			instance(gal_virtual_machine_state& state);

		/**
		 * @brief "!"
		 */
		static void					operator_not(magic_value* values);
		constexpr static const char operator_not_name[] = "!";

		/**
		 * @brief "==(arg_placeholder)"
		 */
		static void					operator_eq(magic_value* values);
		constexpr static const char operator_eq_name[] = "==(_)";

		/**
		 * @brief "!=(arg_placeholder)"
		 */
		static void					operator_not_eq(magic_value* values);
		constexpr static const char operator_not_eq_name[] = "!=(_)";

		/**
		 * @brief "instance_of(arg_placeholder)"
		 */
		static void					operator_instance_of(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_instance_of_name[] = "instance_of(_)";

		/**
		 * @brief "to_string()"
		 */
		static void					operator_to_string(magic_value* values);
		constexpr static const char operator_to_string_name[] = "to_string()";

		/**
		 * @brief "typeof()"
		 */
		static void					operator_typeof(magic_value* values);
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
		constexpr static const char name[]		= "Class";
		constexpr static const auto name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_class&			instance(gal_virtual_machine_state& state);

		/**
		 * @brief "nameof()"
		 */
		static void					operator_nameof(magic_value* values);
		constexpr static const char operator_nameof_name[] = "nameof()";

		/**
		 * @brief "super_type()"
		 */
		static void					operator_super_type(magic_value* values);
		constexpr static const char operator_super_type_name[] = "super_type()";

		/**
		 * @brief "to_string()"
		 */
		static void					operator_to_string(magic_value* values);
		constexpr static const char operator_to_string_name[] = "to_string()";

		/**
		 * @brief "attributes()"
		 */
		static void					operator_attributes(magic_value* values);
		constexpr static const char operator_attributes_name[] = "attributes()";
	};

	/**
	 * @brief Object's metaclass which is a subclass of Class.
	 */
	struct meta_object_metaclass : public meta_class
	{
	private:
		using meta_class::meta_class;

	public:
		constexpr static const char	  name[]	  = "Object metaclass";
		constexpr static const auto	  name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_object_metaclass& instance(gal_virtual_machine_state& state);

		/**
		 * @brief "is_same(arg_placeholder,arg_placeholder)"
		 */
		static void					  operator_is_same(magic_value* values);
		constexpr static const char	  operator_is_same_name[] = "is_same";
	};

	struct meta_boolean : public meta_object
	{
	private:
		using meta_object::meta_object;

	public:
		constexpr static const char name[]			  = "Bool";
		constexpr static const auto name_length		  = sizeof(name) - 1;

		constexpr static const char true_name[]		  = "true";
		constexpr static const char false_name[]	  = "false";
		constexpr static const auto true_name_length  = sizeof(true_name) - 1;
		constexpr static const auto false_name_length = sizeof(false_name) - 1;

		// todo: remove state
		static meta_boolean&		instance(gal_virtual_machine_state& state);

		/**
		 * @brief "!"
		 */
		static void					operator_not(magic_value* values);
		constexpr static const char operator_not_name[] = "!";

		/**
		 * @brief "to_string()"
		 *
		 * todo: remove state
		 */
		static void					operator_to_string(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_to_string_name[] = "to_string()";
	};

	struct meta_fiber : public meta_object
	{
	private:
		using meta_object::meta_object;

	public:
		constexpr static const char name[]		= "Fiber";
		constexpr static const auto name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_fiber&			instance(gal_virtual_machine_state& state);

		/**
		 * @brief "new(arg_placeholder)"
		 */
		static void					operator_new(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_new_name[] = "new(_)";

		/**
		 * @brief "abort(arg_placeholder)"
		 */
		static void					operator_abort(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_abort_name[] = "abort(_)";

		/**
		 * @brief "current()"
		 */
		static void					operator_current(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_current_name[] = "current()";

		/**
		 * @brief "suspend()"
		 */
		static void					operator_suspend(gal_virtual_machine_state& state);
		constexpr static const char operator_suspend_name[] = "suspend()";

		/**
		 * @brief "yield()"
		 */
		static void					operator_yield_no_args(gal_virtual_machine_state& state);
		constexpr static const char operator_yield_no_args_name[] = "yield()";

		/**
		 * @brief "yield(arg_placeholder)"
		 */
		static void					operator_yield_has_args(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_yield_has_args_name[] = "yield(_)";

		/**
		 * @brief "call()"
		 */
		static void					operator_call_no_args(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call_no_args_name[] = "call()";

		/**
		 * @brief "call(arg_placeholder)"
		 */
		static void					operator_call_has_args(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call_has_args_name[] = "call(_)";

		/**
		 * @brief "error()"
		 */
		static void					operator_error(magic_value* values);
		constexpr static const char operator_error_name[] = "error()";

		/**
		 * @brief "done()"
		 */
		static void					operator_done(magic_value* values);
		constexpr static const char operator_done_name[] = "done()";

		/**
		 * @brief "transfer()"
		 */
		static void					operator_transfer_no_args(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_transfer_no_args_name[] = "transfer()";

		/**
		 * @brief "transfer(arg_placeholder)"
		 */
		static void					operator_transfer_has_args(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_transfer_has_args_name[] = "transfer(_)";

		/**
		 * @brief "transfer_error(arg_placeholder)"
		 */
		static void					operator_transfer_error(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_transfer_error_name[] = "transfer_error(_)";

		/**
		 * @brief "try()"
		 */
		static void					operator_try_no_args(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_try_no_args_name[] = "try()";

		/**
		 * @brief "try(arg_placeholder)"
		 */
		static void					operator_try_has_args(gal_virtual_machine_state& state, magic_value* values);
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
		static meta_function&		instance(gal_virtual_machine_state& state);

		/**
		 * @brief "new(arg_placeholder)"
		 */
		static void operator_new(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_new_name[] = "new(_)";

		/**
		 * @brief "arity()"
		 */
		static void operator_arity(magic_value* values);
		constexpr static const char operator_arity_name[] = "arity()";

		/**
		 * @brief "call()"
		 */
		static void operator_call0(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call0_name[] = "call()";

		/**
		 * @brief "call(arg_placeholder)"
		 */
		static void operator_call1(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call1_name[] = "call(_)";

		/**
		 * @brief see common.hpp -> max_parameters
		 */

		/**
		 * @brief "call(arg_placeholder,arg_placeholder)"
		 */
		static void operator_call2(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call2_name[] = "call(_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call3(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call3_name[] = "call(_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call4(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call4_name[] = "call(_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call5(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call5_name[] = "call(_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call6(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call6_name[] = "call(_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call7(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call7_name[] = "call(_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call8(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call8_name[] = "call(_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call9(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call9_name[] = "call(_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call10(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call10_name[] = "call(_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call11(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call11_name[] = "call(_,_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call12(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call12_name[] = "call(_,_,_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call13(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call13_name[] = "call(_,_,_,_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call14(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call14_name[] = "call(_,_,_,_,_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call15(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call15_name[] = "call(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)";

		/**
		 * @brief "call(arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder,arg_placeholder)"
		 */
		static void operator_call16(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_call16_name[] = "call(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)";
	};

	struct meta_null : public meta_object
	{
	private:
		using meta_object::meta_object;

	public:
		constexpr static const char name[] = "Null";
		constexpr static const auto name_length = sizeof(name) - 1;

		constexpr static const char null_name[] = "null";
		constexpr static const auto null_name_length = sizeof(null_name) - 1;

		// todo: remove state
		static meta_null&		instance(gal_virtual_machine_state& state);

		/**
		 * @brief "!"
		 */
		static void					operator_not(magic_value* values);
		constexpr static const char operator_not_name[] = "!";

		/**
		 * @brief "to_string()"
		 *
		 * todo: remove state
		 */
		static void					operator_to_string(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_to_string_name[] = "to_string()";
	};

	struct meta_number : public meta_object
	{
	private:
		using meta_object::meta_object;

	public:
		constexpr static const char name[] = "Number";
		constexpr static const auto name_length = sizeof(name) - 1;

		// todo: remove state
		static meta_number&		instance(gal_virtual_machine_state& state);

		/**
		 * @brief "from_string(arg_placeholder)"
		 */
		static void operator_from_string(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_from_string_name[] = "from_string(_)";

		/**
		 * @brief "infinity()"
		 */
		static void operator_infinity(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_infinity_name[] = "infinity()";

		/**
		 * @brief "nan()"
		 */
		static void operator_nan(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_nan_name[] = "nan()";

		/**
		 * @brief "pi()"
		 */
		static void operator_pi(gal_virtual_machine_state& state, magic_value*values);
		constexpr static const char operator_pi_name[] = "pi()";

		/**
		 * @brief "tau()"
		 */
		static void operator_tau(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_tau_name[] = "tau()";

		/**
		 * @brief "max()"
		 */
		static void operator_max(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_max_name[] = "max()";

		/**
		 * @brief "min()"
		 */
		static void operator_min(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_min_name[] = "min()";

		/**
		 * @brief "max_safe_int()"
		 */
		static void operator_max_safe_int(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_max_safe_int_name[] = "max_safe_int()";

		/**
		 * @brief "min_safe_int()"
		 */
		static void operator_min_safe_int(gal_virtual_machine_state& state, magic_value* values);
		constexpr static const char operator_min_safe_int_name[] = "min_safe_int()";
	};
}// namespace gal

#endif//GAL_LANG_META_HPP
