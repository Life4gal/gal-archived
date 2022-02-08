#pragma once

#ifndef GAL_LANG_OBJECT_METHOD_HPP
#define GAL_LANG_OBJECT_METHOD_HPP

#include <object.hpp>

/**
 * @file object_method.hpp
 * @brief This is about the type 'builtin_function_or_method', not GAL methods.
 */

namespace gal::lang
{
	class gal_type_object_internal_function final : public gal_type_object
	{
	private:
		gal_type_object_internal_function();

	public:
		static gal_type_object_internal_function& type();

		[[nodiscard]] constexpr const char* about() const noexcept override { return "internal function or method\n"; }
	};

	class gal_type_object_internal_method final : public gal_type_object
	{
	private:
		gal_type_object_internal_method();

		static gal_type_object_internal_method& type();

		[[nodiscard]] constexpr const char* about() const noexcept override { return "internal method\n"; }
	};

	class gal_method_define final
	{
	public:
		using internal_function_type = gal_object*(*)(gal_object& self, gal_object* args);
		using internal_function_fast_type = gal_object*(*)(gal_object& self, const gal_object* const* args, gal_size_type num_args);
		using internal_function_pair_arg_type = gal_object*(*)(gal_object& self, gal_object* args, gal_object* pair_args);
		using internal_function_fast_pair_arg_type = gal_object*(*)(gal_object& self, const gal_object* const* args, gal_size_type num_args, gal_object* pair_args);

		using flag_type = std::uint32_t;

		enum class flags : flag_type
		{
			varargs = flag_type{1} << 0,
			pair_args = flag_type{1} << 1,
			// flag_no_arg and flag_an_object_arg must not be combined with the flags above.
			no_args = flag_type{1} << 2,
			an_object_arg = flag_type{1} << 3,
			// is_class and is_static are a little different; these control
			// the construction of methods for a class.  These cannot be used for
			// functions in modules.
			is_class = flag_type{1} << 4,
			is_static = flag_type{1} << 5,
			coexist = flag_type{1} << 6,
			fastcall = flag_type{1} << 7,
			// method means the function stores an
			// additional reference to the class that defines it;
			// both self and class are passed to it.
			// It uses gal_internal_method_object instead of gal_internal_function_object.
			// May not be combined with no_args, an_object_arg,
			// is_class or is_static.
			method = flag_type{1} << 8,
		};

	private:
		const char* name_;
		internal_function_type method_;
		flags flag_;
		const char* document_;

	public:
		gal_method_define(
				const char* name,
				const internal_function_type method,
				const flags flag,
				const char* document = nullptr
				)

			: name_{name},
			  method_{method},
			  flag_{flag},
			  document_{document} { }

		[[nodiscard]] constexpr const char* who_am_i() const noexcept { return name_; }

		[[nodiscard]] constexpr bool check_all_flag(std::same_as<flags> auto ... fs) const noexcept { return utils::check_all_enum_flag(flag_, fs...); }

		[[nodiscard]] constexpr bool check_any_flag(std::same_as<flags> auto ... fs) const noexcept { return utils::check_any_enum_flag(flag_, fs...); }

		constexpr void set_flag(std::same_as<flags> auto ... fs) noexcept { utils::set_enum_flag_set(flag_, fs...); }

		/**
		 * @brief Documentation string.
		 */
		[[nodiscard]] constexpr const char* about() const noexcept { return document_; }

		[[nodiscard]] gal_object* operator()(gal_object& self, gal_object* args);
	};

	class gal_object_internal_function : public gal_object
	{
	protected:
		// Description of the internal function to call
		gal_method_define* methods_;
		// Passed as 'self' arg to the internal, can be nullptr
		gal_object* self_;
		// The __module__ attribute, can be anything
		gal_object* module_;
		// List of weak references
		gal_object* weak_ref_list_;
		gal_type_object::vectorcall_function vectorcall_;

	public:
		gal_object_internal_function(
				gal_method_define* methods,
				gal_object*		   self,
				gal_object*		   module = nullptr);
	};

	class gal_object_internal_method final : public gal_object_internal_function
	{
	private:
		// Class that defines this method
		gal_type_object* owner_;

	public:
		gal_object_internal_method(
				gal_method_define* methods,
				gal_object*		   self,
				gal_object*		   module = nullptr,
				gal_type_object*   owner  = nullptr);
	};
}

#endif // GAL_LANG_OBJECT_METHOD_HPP
