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

	struct meta_boolean : public object_class
	{
	private:
		using object_class::object_class;

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

	struct meta_fiber : public object_class
	{
		// todo: fiber supports functions with more than one parameter.

	private:
		using object_class::object_class;

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
		static void					operator_abort(magic_value* values);
		constexpr static const char operator_abort_name[] = "abort(_)";

		/**
		 * @brief "current()"
		 */
		static void					operator_current(magic_value* values);
		constexpr static const char operator_current_name[] = "current()";

		/**
		 * @brief "suspend()"
		 */
		static void					operator_suspend(magic_value* values);
		constexpr static const char operator_suspend_name[] = "suspend()";

		/**
		 * @brief "yield()"
		 */
		static void					operator_yield_no_args(magic_value* values);
		constexpr static const char operator_yield_no_args_name[] = "yield()";

		/**
		 * @brief "yield(arg_placeholder)"
		 */
		static void					operator_yield_has_args(magic_value* values);
		constexpr static const char operator_yield_has_args_name[] = "yield(_)";
	};
}// namespace gal

#endif//GAL_LANG_META_HPP
