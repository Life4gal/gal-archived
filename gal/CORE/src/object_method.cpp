#include <object_method.hpp>
#include <error.hpp>

namespace gal::lang
{
	gal_type_object_internal_function::gal_type_object_internal_function()
		: gal_type_object{
				&gal_type_object_type::type(),
				"internal_function_or_method",
				utils::set_enum_flag_ret(flags::default_flag, flags::have_gc, flags::have_vectorcall),
				// todo: methods
				nullptr,
				// todo: members
				nullptr,
				// todo: rw_interfaces
				nullptr
		} { }

	gal_type_object_internal_function& gal_type_object_internal_function::type()
	{
		static gal_type_object_internal_function instance{};
		return instance;
	}

	gal_type_object_internal_method::gal_type_object_internal_method()
		: gal_type_object{
				&gal_type_object_type::type(),
				"internal_method",
				flags::invalid,
				// no methods
				nullptr,
				// no members
				nullptr,
				// no rw_interfaces,
				nullptr,
				// no metadata
				nullptr,
				// the base class is gal_type_object_internal_function
				&gal_type_object_internal_function::type(),
		} { }

	gal_type_object_internal_method& gal_type_object_internal_method::type()
	{
		static gal_type_object_internal_method instance{};
		return instance;
	}
}
